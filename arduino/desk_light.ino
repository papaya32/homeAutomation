/* This is hopefully the last update that
fixes night mode problems and some simple
wrong topic issues. Looking good.

//SIGNED//
JACK W. O'REILLY
4 Jun 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "oreilly";
const char* password = "9232616cc8";
const char* mqtt_server = "mqtt.oreillyj.com";
int mqtt_port = 1883;  //changed from previously 1884 as a contingency plan
const char* mqtt_user = "desk";
const char* mqtt_pass = "24518000desk";

const char* versionNum = "1.04";

const char* desk1_com = "osh/bed/desk1/com";
const char* desk2_com = "osh/bed/desk2/com";
const char* desk3_com = "osh/bed/desk3/com";
const char* desk4_com = "osh/bed/desk4/com";
const char* test_com = "osh/all/test/com";  //for testing to see what's online and what version is active
const char* temp_com = "osh/bed/all/temp";  //for temp button wall light
const char* start_com = "osh/all/start";  //command is given on start, this esp will result by publishing all of its stats
const char* allOff = "osh/bed/desk/allOff";  //its own allOff topic, controlled by openHAB, triggered by wall or bed light

const char* desk1_stat = "osh/bed/desk1/stat";
const char* desk2_stat = "osh/bed/desk2/stat";
const char* desk3_stat = "osh/bed/desk3/stat";
const char* desk4_stat = "osh/bed/desk4/stat";
const char* allPub = "osh/all/stat";  //general all purpose human readable topic
const char* test_stat = "osh/all/test/stat";  //publishes ON when test com is called
const char* openhab_test = "osh/bed/desk/openhab";  //publishes ON when reconnects
const char* version_stat = "osh/bed/desk/version";  //publishes version number above when called in test com

bool desk1Stat = LOW; //status variable initializations
bool desk2Stat = LOW;
bool desk3Stat = LOW;
bool desk4Stat = LOW;

bool desk1Temp = LOW;  //status variables for storing previous stat during temp function
bool desk2Temp = LOW;
bool desk3Temp = LOW;
bool desk4Temp = LOW;

int desk1Pin = 2;
int desk2Pin = 4;
int desk3Pin = 5;
int desk4Pin = 13;

void lightSwitch(int, bool);  //called whenever a relay needs to be flipped, arguments are relay number and state to flip to
void publishStats();  //function that simply publishes all of the relay stats (desk1 on, desk2 off, etc)
void tempFunc(int);  //called when temp button is pressed, either turns all off or previously on on
void setup_wifi();
void callback(char*, byte*, unsigned int);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  pinMode(desk1Pin, OUTPUT);
  pinMode(desk2Pin, OUTPUT);
  pinMode(desk3Pin, OUTPUT);
  pinMode(desk4Pin, OUTPUT);

  lightSwitch(1, LOW);  //initialize all relays by flipping off
  lightSwitch(2, LOW);
  lightSwitch(3, LOW);
  lightSwitch(4, LOW);
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Desk Light Pap Version: ");  //for my reference in debugging
  Serial.println(versionNum);
  setup_wifi();  //connect to wifi
  client.setServer(mqtt_server, mqtt_port);  //initialize mqtt server connection
  client.setCallback(callback);
}

void setup_wifi()
{
  delay(10);
  Serial.println();  //all for serial bridge debugging
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  //connect to wifi
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  if (!strcmp(topic, desk1_com))  //if the desk1 com topic was published to..
  {
    if ((char)payload[0] == '1')  //if the message was 1...
    {
      lightSwitch(1, HIGH);  //turn the first relay on
    }
    else  //otherwise..
    {
      lightSwitch(1, LOW);  //turn the relay off
    }
  }
  if (!strcmp(topic, desk2_com))  //same as 1..
  {
    if ((char)payload[0] == '1')
    {
      lightSwitch(2, HIGH);
    }
    else
    {
      lightSwitch(2, LOW);
    }
  }
  if (!strcmp(topic, desk3_com))
  {
    if ((char)payload[0] == '1')
    {
      lightSwitch(3, HIGH);
    }
    else
    {
      lightSwitch(3, LOW);
    }
  }
  if (!strcmp(topic, desk4_com))
  {
    if ((char)payload[0] == '1')
    {
      lightSwitch(4, HIGH);
    }
    else
    {
      lightSwitch(4, LOW);
    }
  }
  if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //if the test com topic was called with a 1..
  {
    client.publish(test_stat, "OSH Desk Lights are Online!");  //publish human readable message
    client.publish(openhab_test, "ON");  //publish ON for openhab to read
    client.publish(version_stat, versionNum);  //publish version for my reference in debuggin/finding the right code
  }
  if (((char)payload[0] == '1') && !strcmp(topic, start_com))  //when start com topic called, when openhab starts typically
  {
    publishStats();  //calls function to publish all of the esp's stats
  }
  if (!strcmp(topic, temp_com))  //if temp button pressed on wall light
  {
    if ((char)payload[0] == '0')  //if it was a zero
    {
      tempFunc(0);  //turning the lights OFF...
    }
    else
    {
      tempFunc(1);  //turning the lights ON...
    }
  }
  if (((char)payload[0] == '1') && !strcmp(topic, allOff))  //if it received the all off topic, specific to this esp
  {
    lightSwitch(1, LOW);  //turn everything off..end of story
    lightSwitch(2, LOW);
    lightSwitch(3, LOW);
    lightSwitch(4, LOW);
  }
}

void reconnect()
{
  while(!client.connected())
  {
    Serial.print("Attempting MQTT Connection...");
    client.connect("ESP8266Desk", mqtt_user, mqtt_pass);  //connect with unique identifier
    if (client.connected())
    {
      Serial.println("Connected");
      client.subscribe(desk1_com);  //subscribe to everything
      client.loop();  //bug in PubSubClient library requires a loop after every subscription
      client.subscribe(desk2_com);
      client.loop();
      client.subscribe(desk3_com);
      client.loop();
      client.subscribe(desk4_com);
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.subscribe(temp_com);
      client.loop();
      client.subscribe(start_com);
      client.loop();
      client.subscribe(allOff);
      client.loop();
      
      client.publish(allPub, "Desk Light Controller just Reconnected");
      client.publish(openhab_test, "ON");  //Let openhab know that we just reconnected
    }
    else
    {
      Serial.print("failed, rc=");  //prints rc code, for debugging mqtt connection failure
      Serial.println(client.state());
      Serial.println("Try again in 5 Seconds");
      if (WiFi.status() != WL_CONNECTED)  //if we're not connected to wifi (possible cause of mqtt failure)
      {
        setup_wifi();
      }
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  yield();
}

void lightSwitch(int light, bool state)  //function that controls the relays
{
  if (light == 1)  //if the first parameter is a 1
  {
    desk1Stat = state;  //update the state of the relay
    digitalWrite(desk1Pin, !state);  //write to the relay (logic level reversed because relays are stupid)
    Serial.print("Desk 1 State: ");  //print to serial bridge
    Serial.println(state);
    if (state)  //if called to go HIGH..
    {
      client.publish(desk1_stat, "ON");  //publishes to openhab that it's now on
    }
    else  //and vice versa
    {
      client.publish(desk1_stat, "OFF");
    }
  }
  else if (light == 2)  //same all the way down
  {
    desk2Stat = state;
    digitalWrite(desk2Pin, !state);
    Serial.print("Desk 2 State: ");
    Serial.println(state);
    if (state)
    {
      client.publish(desk2_stat, "ON");
    }
    else
    {
      client.publish(desk2_stat, "OFF");
    }
  }
  else if (light == 3)
  {
    desk3Stat = state;
    digitalWrite(desk3Pin, !state);
    Serial.print("Desk 3 State: ");
    Serial.println(state);
    if (state)
    {
      client.publish(desk3_stat, "ON");
    }
    else
    {
      client.publish(desk3_stat, "OFF");
    }
  }
  else if (light == 4)
  {
    desk4Stat = state;
    digitalWrite(desk4Pin, !state);
    Serial.print("Desk 4 State: ");
    Serial.println(state);
    if (state)
    {
      client.publish(desk4_stat, "ON");
    }
    else
    {
      client.publish(desk4_stat, "OFF");
    }
  }
}

void tempFunc(int state)  //function that is called when the wall light temp button is pushed (0 turns lights off)
{
  if (!state)  //turning lights off
  {
    desk1Temp = desk1Stat;  //update temporary state variables
    desk2Temp = desk2Stat;
    desk3Temp = desk3Stat;
    desk4Temp = desk4Stat;
    
    lightSwitch(1, LOW);  //turn all of the lights off
    lightSwitch(2, LOW);
    lightSwitch(3, LOW);
    lightSwitch(4, LOW);
  }
  else  //lights back on
  {
    lightSwitch(1, desk1Temp);  //switch lights according to previously recorded temporary variables
    lightSwitch(2, desk2Temp);
    lightSwitch(3, desk3Temp);
    lightSwitch(4, desk4Temp);
  }
}

void publishStats()  //function that publishes all of the stats for all relays
{
  if (desk1Stat)  //if desk 1 is HIGH...
  {
    client.publish(desk1_stat, "ON");  //...print HIGH
  }
  if (!desk1Stat)  //if desk 1 is LOW...
  {
    client.publish(desk1_stat, "OFF");  //...print LOW
  }
  if (desk2Stat)  //that's all there is to it
  {
    client.publish(desk2_stat, "ON");
  }
  if (!desk2Stat)
  {
    client.publish(desk2_stat, "OFF");
  }
  if (desk3Stat)
  {
    client.publish(desk3_stat, "ON");
  }
  if (!desk3Stat)
  {
    client.publish(desk3_stat, "OFF");
  }
  if (desk4Stat)
  {
    client.publish(desk4_stat, "ON");
  }
  if (!desk4Stat)
  {
    client.publish(desk4_stat, "OFF");
  }
}

//PapI 2016
