#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <Servo.h>

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* wall1_com = "osh/bed/wall1/com";  //command mqtt topic
const char* test_com = "osh/all/test/com";  //command mqtt for live testing
const char* nightMode = "osh/bed/nightMode/com";  //command mqtt for night lockMode
const char* temp_com = "osh/bed/all/temp"; //command mqtt for temporary shutdown (leaving room for a few minutes)
const char* lock_com = "osh/bed/lock/com";  //command mqtt for lock

const char* wall1_stat = "osh/bed/wall1/stat";  //status mqtt topic
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing
const char* allPub = "osh/all/stat";  //topic for all esp's to publish their stats
const char* lock_stat = "osh/bed/lock/stat";  //status mqtt for lock status
const char* temp_openhab = "osh/bed/temp/openhab";

#define buttonWall 4  //wall light button pin (defined because used in switch statement)
#define buttonAll 12  //temporary all on/off button pin
#define buttonLock 13  //lock button
#define buttonUnlock 15  //unlock button

bool currentStateButton = LOW;  //initializing boolean status variables (for toggling)
bool lastStateButtonWall = LOW;  //last state of wall "toggle" button
bool lastStateButtonAll = HIGH;  //last state of temp all on/off "toggle" button

bool lightStat = LOW;
bool lockStat = LOW;  //low is unlocked; high locked

int relayPin = 14;  //light relay pin
int ledPin = 16;  //led PIN (used for indicating connecting to wifi and mqtt broker)
int servoPin = 5;  //pin for servo used for locking/unlocking door

#define numButtons 4  //total number of buttons to check for press (must update if button added)
char* buttonArray[numButtons] = {"4", "12", "13", "15"};

int lockDegree = 150;  //max degree of turn for lock servo

void lightOn();  //functiion for turning on lights
void lightOff();  //turning lights off
void dimmer();  //called when mqtt disconnected, slowly turns on and off the button LED using PWM
void buttonPress();  //function that listens for button pushes, called in main loop and in dimmer function
void lockDoor(int);  //function to lock door; parameter is lock/unlock (1 for lock, 0 for unlock)
void setup_wifi();  //initializes wifi setup function
void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

Servo lockServo;  //initializes lock servo

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin
  pinMode(buttonWall, INPUT);  //all button input pins
  pinMode(buttonAll, INPUT);
  pinMode(buttonLock, INPUT);
  pinMode(buttonUnlock, INPUT);

  lightOff();  //initializes relay and LED pins
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println();
  Serial.println("Wall Light Pap Version 1.0!!");  //for my reference
  lightOff();  //initializes LED and relay pins (after wifi blinking probably screwed it up)
  setup_wifi();  //starts up wifi (user defined function)
  client.setServer(mqtt_server, 1883);  //connects to mqtt server (second parameter is port)
  client.setCallback(callback);  //sets callback function (as callback function is user defined)
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  //starts wifi connection

  while (WiFi.status() != WL_CONNECTED) {  //while wifi not connected...
    Serial.print(".");
    analogWrite(ledPin, 0);  //turn LED off
    int i;
    for (i = 0; i <= 50; i++)
    {
      buttonPress();
      delay(10);
    }
    analogWrite(ledPin, 1023);  //turn LED on
    for (i = 0; i <= 50; i++)
    {
      buttonPress();
      delay(50);
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  //prints local ip address
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {  //loop that prints out each character in char array that is message
    Serial.print((char)payload[i]);  //prints out array element by element
  }
  Serial.println();

  //Note for the next few lines, strcmp() returns zero if the two CHAR ARRAYS equal each other, contrary to logical thought
  if (((char)payload[0] == '1') && !strcmp(topic, wall1_com))  //if message is one and topics match...
  {
    lightOn();  //turn the lights on
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, wall1_com))
  {
    lightOff();  //turn the lights off
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //condition for live testing
  {
    client.publish(test_stat, "OSH Bed Wall Light is Live!");  //publishes that this esp is live
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, nightMode))
  {
    lightOff();
    client.publish(allPub, "OSH Bed Wall Light is OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, lock_com))  //if command for door lock
  {
    lockDoor(1);  //lock door (1 is parameter for lock)
    client.publish(lock_stat, "ON");  //publish locked for openHAB
    client.publish(allPub, "Bedroom Door is Locked");  //status topic for everyone
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, lock_com))  //if command for unlock
  {
    lockDoor(0);  //unlock door
    client.publish(lock_stat, "OFF");
    client.publish(allPub, "Bedroom Door is Unlocked");
  }
}

void reconnect() {  //this function is called repeatedly until mqtt is connected to broker
  while (!client.connected())  //while not connected...
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266Client");  //connect funtion, must include ESP8266Client
    if (client.connected())  //if connected...
    {
      Serial.println("connected");
      if (digitalRead(relayPin) == LOW)  //if light is on... (this part makes sure button LED matches light state after dimmer function)
      {
        analogWrite(ledPin, 0);  //turn button LED off
      }
      else  //otherwise...
      {
        analogWrite(ledPin, 1023);  //turn button LED on
      }
      client.subscribe(wall1_com);  //once connected, subscribe
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.subscribe(nightMode);
      client.loop();
      client.subscribe(lock_com);
      client.loop();
	  
	  client.publish(allPub, "Wall Light just reconnected!");
    }
    else  //if we're not connected
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)  //if wifi is disconnected (could be reason for mqtt not connecting)
      {
        setup_wifi();  //calls wifi setup function
      }
      dimmer();  //dim the button LED on and off (around 5 second delay)
    }
  }
}
void loop() {

  if (!client.connected())  //if not connected to mqtt...
  {
    reconnect();  //call reconnect function
  }
  client.loop();  //keeps searching subscriptions, absolutely required
  buttonPress();  //checks for button presses of all buttons
}

void lightOn()  //lights on function
{
  digitalWrite(relayPin, LOW);   // Turns on relay
  lightStat = LOW;  //updates status for temporary on/off button call
  analogWrite(ledPin, 0);  //turns off button LED
  client.publish(wall1_stat, "ON");  //publishes status
  Serial.println("LIGHT ON");  //prints status for debugging
}

void lightOff() //lights off function
{
  digitalWrite(relayPin, HIGH);  //turn off relay
  lightStat = HIGH;  //updates status for temporary on/off button
  analogWrite(ledPin, 1023);  //turn on button LED
  client.publish(wall1_stat, "OFF");  //publishes status
  Serial.println("LIGHT OFF");  //prints to serial monitor
}

void dimmer()  //dimmer function (for recognizing disconnect when in wall)
{
  int i;
  int pos;
  for (i = 0; i < 1023; i++) //loop from dark to bright in ~3.1 seconds
  {
    analogWrite(ledPin, i);  //set PWM rate
    buttonPress();  //call button press function (so switch can be used when offline)
    delay(3);  //ideal timing for dimming rate
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    buttonPress();
    delay(3);
  }
}

void lockDoor(int lockMode)  //door lock function
{
  lockServo.attach(servoPin);  //attaches to servo pin
  int i;
  delay(10);
  if (lockMode)  //if parameter is a 1
  {
    for (i = 0; i <= lockDegree; i++)  //start for loop for each degree of turn
    {
      lockServo.write(i);  //write degree to servo
      delay(5);  //delay so servo doesn't blow up
    }
    Serial.println("Door locked!");
  }
  else if (!lockMode)  //if 0 (unlock)
  {
    for (i = lockDegree; i >= 0; i--)
    {
      lockServo.write(i);
      delay(5);
    }
    Serial.println("Door unlocked!");
  }
  lockServo.detach();  //detaches so the servo isn't super stiff and the key can be turned (just in case)
}

void buttonPress()  //function that
{
  int i;
  for (i = 0; i < numButtons; i++)  //for loop that checks each button
  {
    int currentButton = atoi(buttonArray[i]);
    currentStateButton = digitalRead(currentButton);  //current state is reading the state of the button
    if (currentStateButton)  //if the button is currently being pressed...
    {
      switch (currentButton)  //switch statement where the argument is the pin number that is currently being pushed
      {
        case buttonLock:  //if it's the lock button...
          lockDoor(1);  //lock the door
          client.publish(lock_stat, "ON");  //publish that the door's locked
          delay(20);  //"debounce"
          break;
        case buttonUnlock:  //if unlock button..
          lockDoor(0);
          client.publish(lock_stat, "OFF");
          delay(20);
          break;
        case buttonWall:  //if it's the button to control the ceiling light
          if (lastStateButtonWall)  //if the last state was HIGH
          {
            lightOff();  //turn the light off
            //delay(20);
            lastStateButtonWall = LOW;  //update current state of ceiling light
          }
          else  //otherwise (last state was off)
          {
            lightOn();
            delay(20);
            lastStateButtonWall = HIGH;  //update current state of light to on
          }
          break;
        case buttonAll:  //if it's the temporary on/off button press
          if (lastStateButtonAll)  //if the last state was HIGH (meaning we're now turning the lights off)
          {
            client.publish(temp_com, "0");  //publish to all esp's to turn off
            client.publish(temp_openhab, "ON");
            bool tempLightStat = lightStat;
            lightOff();
            delay(20);
            lastStateButtonAll = LOW;  //update last all state to LOW
            lightStat = tempLightStat;
          }
          else
          {
            client.publish(temp_com, "1");
            client.publish(temp_openhab, "OFF");
            if (!lightStat)  //if the last state of the ceiling light was off...
            {
              lightOn();
            }
            delay(20);
            lastStateButtonAll = HIGH;
          }
          break;
      }
      while (digitalRead(currentButton))  //while the current button is still being pressed...
      {
        delay(5);  //delay so the esp doesn't blow up/crash :)
      }
      delay(20);
    }
  }
}
