/* This version (v1.1) of the wall light sketch
is the most improved version, and should address
most of or all of the bugs that were found in
the previous revision, including the necessity
to occassionally push the toggle button multiple
times in order to toggle the light, due to
a faulty if statement and the status variables
and the actual states being unaligned.
Additionally, code for better integration with
openHAB has been added, with:

1. Status updates of all control variables when
prompted

2. Post whenever the ESP reconnects

//SIGNED//
JACK W. O'REILLY
11 Mar 2016*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* wall1_com = "osh/bed/wall1/com";  //command mqtt topic
const char* test_com = "osh/all/test/com";  //command mqtt for live testing
const char* nightMode = "osh/bed/nightMode/com";  //command mqtt for night lockMode
const char* temp_com = "osh/bed/all/temp"; //command mqtt for temporary shutdown (leaving room for a few minutes)
const char* start_com = "osh/all/start";

const char* wall1_stat = "osh/bed/wall1/stat";  //status mqtt topic
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing
const char* allPub = "osh/all/stat";  //topic for all esp's to publish their stats
const char* temp_openhab = "osh/bed/temp/openhab";
const char* openhab_test = "osh/bed/wall1/openhab";
const char* allOff = "osh/bed/all/allOff";

#define buttonWall 4  //wall light button pin (defined because used in switch statement)
#define buttonAll 12  //temporary all on/off button pin

bool currentStateButton = LOW;  //initializing boolean status variables (for toggling)
bool lastStateButtonAll = HIGH;  //last state of temp all on/off "toggle" button

bool lightStat = LOW;
bool lockStat = LOW;  //low is unlocked; high locked

int relayPin = 14;  //light relay pin
int ledPin = 16;  //led PIN (used for indicating connecting to wifi and mqtt broker)

#define numButtons 2  //total number of buttons to check for press (must update if button added)
char* buttonArray[numButtons] = {"4", "12"};

unsigned long int doublePressTime = 350;

void lightSwitch(int);
void dimmer();  //called when mqtt disconnected, slowly turns on and off the button LED using PWM
void buttonPress();  //function that listens for button pushes, called in main loop and in dimmer function
void setup_wifi();  //initializes wifi setup function
void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin
  pinMode(buttonWall, INPUT_PULLUP);  //all button input pins
  pinMode(buttonAll, INPUT_PULLUP);

  lightSwitch(0);  //initializes relay and LED pins
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println();
  Serial.println("Wall Light Pap Version 1.1!!");  //for my reference
  setup_wifi();  //starts up wifi (user defined function)
  lightSwitch(0);  //initializes LED and relay pins (after wifi blinking probably screwed it up)
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
    lightSwitch(1);  //turn the lights on
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, wall1_com))
  {
    lightSwitch(0);  //turn the lights off
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //condition for live testing
  {
    client.publish(test_stat, "OSH Bed Wall Light is Live!");  //publishes that this esp is live
	client.publish(openhab_test, "ON");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, nightMode))
  {
    lightSwitch(0);
    client.publish(allPub, "OSH Bed Wall Light is OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, start_com))
  {
    if (lightStat)
	{
	  client.publish(wall1_stat, "ON");
	}
	if (!lightStat)
	{
      client.publish(wall1_stat, "OFF");
	}
	if (lastStateButtonAll)
	{
	  client.publish(temp_com, "ON");
	}
	if (!lastStateButtonAll)
	{
	  client.publish(temp_com, "OFF");
	}
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
	  
	  client.publish(allPub, "Wall Light just reconnected!");
	  client.publish(openhab_test, "ON");
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
  yield();
  buttonPress();  //checks for button presses of all buttons
}

void lightSwitch(int mode)
{
  if (mode)
  {
    digitalWrite(relayPin, LOW);
	lightStat = LOW;
	analogWrite(ledPin, 0);
	client.publish(wall1_stat, "ON");
	Serial.println("Light On");
  }
  else
  {
    digitalWrite(relayPin, HIGH);
	lightStat = HIGH;
	analogWrite(ledPin, 1023);
	client.publish(wall1_stat, "OFF");
	Serial.println("Light Off");
  }
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

void buttonPress()  //function that
{
  int i;
  for (i = 0; i < numButtons; i++)  //for loop that checks each button
  {
    int currentButton = atoi(buttonArray[i]);
    currentStateButton = digitalRead(currentButton);  //current state is reading the state of the button
    if (!currentStateButton)  //if the button is currently being pressed...
    {
      switch (currentButton)  //switch statement where the argument is the pin number that is currently being pushed
      {
        case buttonWall:  //if it's the button to control the ceiling light
		  lastStateButtonAll = HIGH;
		  client.publish(temp_openhab, "ON");
          if (lightStat)  //if the last state was HIGH
          {
            lightSwitch(0);  //turn the light off
            //delay(20);
            lightStat = LOW;  //update current state of ceiling light
          }
          else  //otherwise (last state was off)
          {
            lightSwitch(1);
            delay(20);
            lightStat = HIGH;  //update current state of light to on
          }
          break;
        case buttonAll:  //if it's the temporary on/off button press
          if (lastStateButtonAll)  //if the last state was HIGH (meaning we're now turning the lights off)
          {
            client.publish(temp_com, "0");  //publish to all esp's to turn off
            client.publish(temp_openhab, "ON");
            bool tempLightStat = lightStat;
            lightSwitch(0);
			yield();
            lastStateButtonAll = LOW;  //update last all state to LOW
            lightStat = tempLightStat;
          }
          else
          {
            client.publish(temp_com, "1");
            client.publish(temp_openhab, "OFF");
            if (!lightStat)  //if the last state of the ceiling light was off...
            {
              lightSwitch(1);
            }
            yield();
            lastStateButtonAll = HIGH;
          }
		  while (!digitalRead(currentButton))
		  {
		    delay(5);
			yield();
		  }
		  unsigned long int timeRelease = millis();
		  bool test = LOW;
		  while (((millis() - timeRelease) <= doublePressTime) && (test == LOW))
		  {
		    delay(5);
			yield();
			if (!digitalRead(currentButton))
			{
			  test = HIGH;
			  client.publish(allOff, "ON");
			  lastStateButtonAll = HIGH;
			}
		  }
		  yield();
          break;
      }
      while (!digitalRead(currentButton))  //while the current button is still being pressed...
      {
		delay(5);
        yield();		//delay so the esp doesn't blow up/crash :)
      }
    }
  }
}
