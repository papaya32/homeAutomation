/* Desk Light Round ONE!!

//SIGNED//
JACK W. O'REILLY
23 Apr 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "oreilly";
const char* password = "9232616cc8";
const char* mqtt_server = "mqtt.oreillyj.com";
int mqtt_port = 1883;
const char* mqtt_user = "desk";
const char* mqtt_pass = "24518000desk";

const char* versionNum = "0.50";

const char* desk1_com = "osh/bed/desk1/com";
const char* desk2_com = "osh/bed/desk2/com";
const char* desk3_com = "osh/bed/desk3/com";
const char* desk4_com = "osh/bed/desk4/com";
const char* test_com = "osh/all/test/com";
const char* nightMode = "osh/bed/nightMode/com";
const char* temp_com = "osh/bed/all/temp";
const char* start_com = "osh/all/start";
const char* allOff = "osh/bed/desk/allOff";

const char* desk1_stat = "osh/bed/desk1/stat";
const char* desk2_stat = "osh/bed/desk2/stat";
const char* desk3_stat = "osh/bed/desk3/stat";
const char* desk4_stat = "osh/bed/desk4/stat";
const char* allPub = "osh/all/stat";
const char* openhab_test = "osh/bed/desk/openhab";
const char* version_stat = "osh/bed/desk/version";

bool desk1Stat = LOW;
bool desk2Stat = LOW;
bool desk3Stat = LOW;
bool desk4Stat = LOW;

bool desk1Temp = LOW;
bool desk2Temp = LOW;
bool desk3Temp = LOW;
bool desk4Temp = LOW;

int desk1Pin = 2;
int desk2Pin = 4;
int desk3Pin = 5;
int desk4Pin = 13;
int nightPin = 12;

void lightSwitch(int, bool);
void publishStats();
void tempFunc(int);
void setup_wifi();
void callback(char*, byte*, unsigned int);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  pinMode(desk1Pin, INPUT_PULLUP);
  pinMode(desk2Pin, INPUT_PULLUP);
  pinMode(desk3Pin, INPUT_PULLUP);
  pinMode(desk4Pin, INPUT_PULLUP);
  pinMode(nightPin, OUTPUT);
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Desk Light Pap Version: ");
  Serial.println(versionNum);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
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
  if (!strcmp(topic, desk1_com))
  {
    if ((char)payload[0] == '1')
    {
      lightSwitch(1, HIGH);
    }
    else
    {
      lightSwitch(1, LOW);
    }
  }
  if (!strcmp(topic, desk2_com))
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
  if (((char)payload[0] == '1') && !strcmp(topic, test_com))
  {
    client.publish(allPub, "OSH Desk Lights are Online!");
    client.publish(openhab_test, "ON");
    client.publish(version_stat, versionNum);
  }
  if (((char)payload[0] == '1') && !strcmp(topic, nightMode))
  {
    digitalWrite(desk1Pin, HIGH);
    digitalWrite(desk2Pin, HIGH);
    digitalWrite(desk3Pin, HIGH);
    digitalWrite(desk4Pin, HIGH);
    digitalWrite(nightPin, LOW);
    client.publish(allPub, "OSH Bed Desk in Night Mode");
  }
  if (((char)payload[0] == '1') && !strcmp(topic, start_com))
  {
    publishStats();
  }
  if (!strcmp(topic, temp_com))
  {
    if ((char)payload[0] == '0')
    {
      tempFunc(0);
    }
    else
    {
      tempFunc(1);
    }
  }
  if (((char)payload[0] == '1') && !strcmp(topic, allOff))
  {
    lightSwitch(1, LOW);
    lightSwitch(2, LOW);
    lightSwitch(3, LOW);
    lightSwitch(4, LOW);
  }
}

void reconnect()
{
  while(client.connected())
  {
    Serial.print("Attempting MQTT Connection...");
    client.connect("ESP8266Desk", mqtt_user, mqtt_pass);
    if (client.connected())
    {
      Serial.println("Connected");
      client.subscribe(desk1_com);
      client.loop();
      client.subscribe(desk2_com);
      client.loop();
      client.subscribe(desk3_com);
      client.loop();
      client.subscribe(desk4_com);
      client.loop();
      client.subscribe(nightMode);
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
      client.publish(openhab_test, "ON");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println("Try again in 5 Seconds");
      if (WiFi.status() != WL_CONNECTED)
      {
        setup_wifi();
      }
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

void lightSwitch(int light, bool state)
{
  if (light == 1)
  {
    desk1Stat = state;
    digitalWrite(desk1Pin, !state);
    Serial.print("Desk 1 State: ");
    Serial.println(state);
    if (state)
    {
      client.publish(desk1_stat, "ON");
    }
    else
    {
      client.publish(desk1_stat, "OFF");
    }
  }
  else if (light == 2)
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

void tempFunc(int state)
{
  if (!state)  //turning lights off
  {
    desk1Temp = desk1Stat;
    desk2Temp = desk2Stat;
    desk3Temp = desk3Stat;
    desk4Temp = desk4Stat;
    
    lightSwitch(1, LOW);
    lightSwitch(2, LOW);
    lightSwitch(3, LOW);
    lightSwitch(4, LOW);
  }
  else  //lights back on
  {
    lightSwitch(1, desk1Temp);
    lightSwitch(2, desk2Temp);
    lightSwitch(3, desk3Temp);
    lightSwitch(4, desk4Temp);
  }
}

void publishStats()
{
  if (desk1Stat)
  {
    client.publish(desk1_stat, "ON");
  }
  else if (!desk1Stat)
  {
    client.publish(desk1_stat, "OFF");
  }
  if (desk2Stat)
  {
    client.publish(desk2_stat, "ON");
  }
  else if (!desk2Stat)
  {
    client.publish(desk2_stat, "OFF");
  }
  if (desk3Stat)
  {
    client.publish(desk3_stat, "ON");
  }
  else if (!desk3Stat)
  {
    client.publish(desk3_stat, "OFF");
  }
  if (desk4Stat)
  {
    client.publish(desk4_stat, "ON");
  }
  else if (!desk4Stat)
  {
    client.publish(desk4_stat, "OFF");
  }
}
