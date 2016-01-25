/*Button function calls and function itself need to be uncommented
   and tested fully. Additionally, the nightMode must be tested.
   This should include 4 physical switches all wired such that those
   that are on allow the nightModePin signal through to ALL relays.
   After that, the esp should be ready to deploy :D
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* desk1_com = "osh/bed/desk1/com";  //command mqtt topic
const char* desk2_com = "osh/bed/desk2/com";
const char* desk3_com = "osh/bed/desk3/com";
const char* desk4_com = "osh/bed/desk4/com";
const char* test_com = "osh/all/test/com";  //command mqtt for live testing
const char* nightMode = "osh/bed/nightMode/com";  //command mqtt for night mode
const char* temp_com = "osh/bed/all/temp";

const char* desk1_stat = "osh/bed/desk1/stat";  //status mqtt topic
const char* desk2_stat = "osh/bed/desk2/stat";
const char* desk3_stat = "osh/bed/desk3/stat";
const char* desk4_stat = "osh/bed/desk4/stat";
const char* nightMode_stat = "osh/bed/nightMode/stat";
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing
const char* allPub = "osh/all/stat";  //topic for all esp's to publish their stats

int desk1Pin = 14;  //initializes relay pins
int desk2Pin = 16;
int desk3Pin = 12;
int desk4Pin = 13;
int nightModePin = 5;

int desk1Button = 4;
int desk2Button = 15;
int desk3Button = 2;
int desk4Button = 0;

int roundOne = 1;

void pushTest(int, int, bool*, const char*);

bool currentStateButton = LOW;
bool lastStateButton = LOW;

bool desk1Stat = HIGH;
bool desk2Stat = HIGH;
bool desk3Stat = HIGH;
bool desk4Stat = HIGH;
bool nightModeStat = HIGH;

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void setup_wifi();  //initializes wifi setup function
void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

void setup() {
  pinMode(desk1Pin, OUTPUT);
  pinMode(desk2Pin, OUTPUT);
  pinMode(desk3Pin, OUTPUT);
  pinMode(desk4Pin, OUTPUT);
  pinMode(nightModePin, OUTPUT);
  digitalWrite(desk1Pin, desk1Stat);
  digitalWrite(desk2Pin, desk2Stat);
  digitalWrite(desk3Pin, desk3Stat);
  digitalWrite(desk4Pin, desk4Stat);
  digitalWrite(nightModePin, nightModeStat);
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println("Desk Light Pap Version 0.9");  //for my reference
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
    delay(200);
  /*pushTest(desk1Button, desk1Pin, &desk1Stat, desk1_stat);
    pushTest(desk2Button, desk2Pin, &desk2Stat, desk2_stat);
    pushTest(desk3Button, desk3Pin, &desk3Stat, desk3_stat);*/
  pushTest(desk4Button, desk4Pin, &desk4Stat, desk4_stat);
    Serial.print(".");
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
  if (((char)payload[0] == '1') && !strcmp(topic, desk1_com))  //if message is one and topics match...
  {
    digitalWrite(desk1Pin, LOW);  //turn the lights on
    desk1Stat = LOW;
    client.publish(desk1_stat, "ON");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, desk1_com))
  {
    digitalWrite(desk1Pin, HIGH);  //turn the lights off
    desk1Stat = HIGH;
    client.publish(desk1_stat, "OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, desk2_com))  //if message is one and topics match...
  {
    digitalWrite(desk2Pin, LOW);  //turn the lights on
    desk2Stat = LOW;
    client.publish(desk2_stat, "ON");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, desk2_com))
  {
    digitalWrite(desk2Pin, HIGH);  //turn the lights off
    desk2Stat = HIGH;
    client.publish(desk2_stat, "OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, desk3_com))  //if message is one and topics match...
  {
    digitalWrite(desk3Pin, LOW);  //turn the lights on
    desk3Stat = LOW;
    client.publish(desk3_stat, "ON");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, desk3_com))
  {
    digitalWrite(desk3Pin, HIGH);  //turn the lights off
    desk3Stat = HIGH;
    client.publish(desk3_stat, "OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, desk4_com))  //if message is one and topics match...
  {
    digitalWrite(desk4Pin, LOW);  //turn the lights on
    desk4Stat = LOW;
    client.publish(desk4_stat, "ON");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, desk4_com))
  {
    digitalWrite(desk4Pin, HIGH);  //turn the lights off
    desk4Stat = HIGH;
    client.publish(desk4_stat, "OFF");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //condition for live testing
  {
    client.publish(test_stat, "OSH Bed Desk Lights are Online!");  //publishes that this esp is live
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, nightMode))
  {
    digitalWrite(desk1Pin, HIGH);
    digitalWrite(desk2Pin, HIGH);
    digitalWrite(desk3Pin, HIGH);
    digitalWrite(desk4Pin, HIGH);
    digitalWrite(nightModePin, LOW);
    desk1Stat = HIGH;
    desk2Stat = HIGH;
    desk3Stat = HIGH;
    desk4Stat = HIGH;
    nightModeStat = LOW;
    client.publish(allPub, "OSH Bed Desk Night Mode is ON");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, temp_com))
  {
    digitalWrite(desk1Pin, HIGH);
    digitalWrite(desk2Pin, HIGH);
    digitalWrite(desk3Pin, HIGH);
    digitalWrite(desk4Pin, HIGH);
    digitalWrite(nightModePin, HIGH);
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, temp_com))
  {
    digitalWrite(desk1Pin, desk1Stat);
    digitalWrite(desk2Pin, desk2Stat);
    digitalWrite(desk3Pin, desk3Stat);
    digitalWrite(desk4Pin, desk4Stat);
    digitalWrite(nightModePin, nightModeStat);
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
      client.subscribe(desk1_com);  //once connected, subscribe
      client.loop();
      client.subscribe(desk2_com);
      client.loop();
      client.subscribe(desk3_com);
      client.loop();
      client.subscribe(desk4_com);
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.subscribe(nightMode);
      client.loop();
      client.subscribe(temp_com);
      client.loop();
    }
    else
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      int i;
      for (i=0;i<1000;i++)
      {
      /*pushTest(desk1Button, desk1Pin, &desk1Stat, desk1_stat);
        pushTest(desk2Button, desk2Pin, &desk2Stat, desk2_stat);
        pushTest(desk3Button, desk3Pin, &desk3Stat, desk3_stat);*/
        pushTest(desk4Button, desk4Pin, &desk4Stat, desk4_stat);
        delay(5);
      }
    }
  }
}
void loop() {

  if (!client.connected())  //if not connected to mqtt...
  {
    reconnect();  //call reconnect function
  }
  client.loop();  //keeps searching subscriptions, absolutely required
  /*pushTest(desk1Button, desk1Pin, &desk1Stat, desk1_stat);
    pushTest(desk2Button, desk2Pin, &desk2Stat, desk2_stat);
    pushTest(desk3Button, desk3Pin, &desk3Stat, desk3_stat);*/
  pushTest(desk4Button, desk4Pin, &desk4Stat, desk4_stat);

  if (roundOne)
  {
    client.publish(desk1_stat, "OFF");
    client.publish(desk2_stat, "OFF");
    client.publish(desk3_stat, "OFF");
    client.publish(desk4_stat, "OFF");
    client.publish(nightMode_stat, "OFF");
  }
  roundOne = 0;
}

void pushTest(int buttonPin, int relayPin, bool *state, const char* deskStat)
{
  currentStateButton = digitalRead(buttonPin);  //current state is read from pin
  if (currentStateButton == HIGH && lastStateButton == LOW) //if button is being pressed, and was previously off
  {
    digitalWrite(relayPin, LOW);
    client.publish(deskStat, "ON");
    *state = LOW;
    delay(15);  //rudimentary debounce
    lastStateButton = HIGH;  //set "previous" state
    while (digitalRead(buttonPin) == HIGH)  //while button is being pressed
    {
      delay(5);  //keeps the esp from crashing :)
    }
  }
  else if (currentStateButton == HIGH && lastStateButton == HIGH) //just toggled off
  {
    digitalWrite(relayPin, HIGH);
    client.publish(deskStat, "OFF");
    *state = HIGH;
    delay(15);
    lastStateButton = LOW;
    while (digitalRead(buttonPin) == HIGH)  //while button is being pressed
    {
      delay(5);//yayy
    }
  }
}
