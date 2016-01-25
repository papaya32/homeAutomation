#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <Servo.h>

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* wall1_com = "osh/bed/wall1/com";  //command mqtt topic
const char* test_com = "osh/all/test/com";  //command mqtt for live testing
const char* nightMode = "osh/bed/nightMode/com";  //command mqtt for night mode
const char* temp_com = "osh/bed/all/temp";
const char* lock_com = "osh/bed/lock/com";

const char* wall1_stat = "osh/bed/wall1/stat";  //status mqtt topic
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing
const char* allPub = "osh/all/stat";  //topic for all esp's to publish their stats
const char* temp_stat = "osh/bed/all/tempStat";
const char* lock_stat = "osh/bed/lock/stat";

bool currentStateButton = LOW;  //initializing boolean status variables (for toggling)
bool lastStateButton = LOW;
bool currentStateButtonAll = LOW;
bool lastStateButtonAll = LOW;

bool lightStat = HIGH;

int relayPin = 14;
int ledPin = 16;
int servoPin = 5;
int buttonPin = 4;
int buttonPinAll = 12;
int lockDegree = 150;

void lightOn();  //functiion for turning on lights
void lightOff();  //turning lights off
void dimmer();  //called when mqtt disconnected, slowly turns on and off the button LED using PWM
void pushTest();  //function that listens for button push, called in main loop and in dimmer function
void pushTestAll();
void lockDoor(int);

Servo lockServo;

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void setup_wifi();  //initializes wifi setup function
void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin
  pinMode(buttonPin, INPUT);  //button
  lightOff();  //initializes relay and LED pins
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println("Wall Light Pap Version 0.9");  //for my reference
  setup_wifi();  //starts up wifi (user defined function)
  lightOff();  //initializes LED and relay pins (after wifi blinking probably screwed it up)
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
    delay(500);  //wait half a second
    analogWrite(ledPin, 1023);  //turn LED on
    delay(500);  //wait again
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
  else if (((char)payload[0] == '1') && !strcmp(topic, lock_com))
  {
    lockDoor(1);
    client.publish(lock_stat, "ON");
    client.publish(allPub, "Bedroom Door is Locked");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, lock_com))
  {
    lockDoor(0);
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

    }
    else
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)
      {
        setup_wifi();
      }
      dimmer();  //dim the button LED on and off
    }
  }
}
void loop() {

  if (!client.connected())  //if not connected to mqtt...
  {
    reconnect();  //call reconnect function
  }
  client.loop();  //keeps searching subscriptions, absolutely required
  //pushTest();  //checks for button press
  //pushTestAll();
}

void lightOn()  //lights on function
{
  digitalWrite(relayPin, LOW);   // Turns on relay
  lightStat = LOW;
  analogWrite(ledPin, 0);  //turns off button LED
  client.publish(wall1_stat, "ON");  //publishes status
  Serial.println("LIGHT ON");  //prints status for debugging
}

void lightOff() //lights off function
{
  digitalWrite(relayPin, HIGH);  //turn off relay
  lightStat = HIGH;
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
    pushTest();  //call button press function (so switch can be used when offline)
    pushTestAll();
    delay(3);  //ideal timing for dimming rate
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    pushTest();
    pushTestAll();
    delay(3);
  }
}


void pushTest()  //function to test for button press
{
  currentStateButton = digitalRead(buttonPin);  //current state is read from pin
  if (currentStateButton == HIGH && lastStateButton == LOW) //if button is being pressed, and was previously off
  {
    lightOn();  //turn lights on
    delay(20);  //rudimentary debounce
    lastStateButton = HIGH;  //set "previous" state
    while (digitalRead(buttonPin) == HIGH)  //while button is being pressed
    {
      delay(5);  //keeps the esp from crashing :)
    }
  }
  else if (currentStateButton == HIGH && lastStateButton == HIGH) //just toggled off
  {
    lightOff();
    delay(20);
    lastStateButton = LOW;
    while (digitalRead(buttonPin) == HIGH)  //while button is being pressed
    {
      delay(5);//yayy
    }
  }
}

void pushTestAll()  //function to test for button press
{
  currentStateButtonAll = digitalRead(buttonPinAll);  //current state is read from pin
  if (currentStateButtonAll == HIGH && lastStateButtonAll == LOW) //if button is being pressed, and was previously off
  {
    client.publish(temp_com, "1");
    client.publish(temp_stat, "OFF");
    if (!lightStat)
    {
      lightOn();
    }
    delay(20);  //rudimentary debounce
    lastStateButtonAll = HIGH;  //set "previous" state
    while (digitalRead(buttonPinAll) == HIGH)  //while button is being pressed
    {
      delay(5);  //keeps the esp from crashing :)
    }
  }
  else if (currentStateButtonAll == HIGH && lastStateButtonAll == HIGH) //just toggled off
  {
    client.publish(temp_com, "0");
    client.publish(temp_stat, "ON");
    lightOff();
    delay(20);
    lastStateButtonAll = LOW;
    while (digitalRead(buttonPinAll) == HIGH)  //while button is being pressed
    {
      delay(5);//yayy
    }
  }
}

void lockDoor(int mode)
{
  lockServo.attach(servoPin);
  int i;
  if (mode)
  {
    for (i = 0; i <= lockDegree; i++)
    {
      lockServo.write(i);
      delay(8);
    }
  }
  else if (!mode)
  {
    for (i = lockDegree; i >= 0; i--)
    {
      lockServo.write(i);
      delay(8);
    }
  }
  lockServo.detach();
}
