/* This is the first attempt at a thermostat
controller. It puts the responsibility of
controlling the relays (and all of the GPIO
pins) in the hands of openHAB. Right now
it doesn't have code for the thermometer, which
will likely use the ADC pin and transmit to
openHAB periodically, perhaps every minute.
At this time, it has not yet been compiled or
debugged.

Version Notes:

-No thermometer code

//SIGNED//
JACK W. O'REILLY
15 May 2016*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>

const char* ssid = "oreilly";
const char* password = "9232616cc8";
const char* mqtt_server = "mqtt.oreillyj.com";
int mqtt_port = 1883;
const char* mqtt_user = "thermo";
const char* mqtt_pass = "24518000thermo";

const char* versionNum = "0.75";

const char* gpio_com = "osh/liv/thermoGPIO/com";
const char* test_com = "osh/all/test/com";

const char* gpio_stat = "osh/lib/thermoGPIO/stat";
const char* test_stat = "osh/all/test/stat";
const char* openhab_test = "osh/lib/thermo/openhab";
const char* version_stat = "osh/lib/thermo/version";
const char* allPub = "osh/all/stat";

void publishStats();
void setup_wifi();
void callback(char*, byte*, unsigned int);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
  digitalWrite(15, LOW);
  digitalWrite(16, LOW);
  digitalWrite(14, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);

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
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int lenth)
{

  if (!strcmp(topic, gpio_com))
  {
    char statechar[1] = payload[2];
    char pinchar[2];
    for (i=0; i<=1; i++)
    {
      pinchar[i] = payload[i];
    }
    int pin = atoi(pinchar);
    (statechar == '0') ? (bool state = LOW) : (bool state = HIGH)
    digitalWrite(pin, state);
    client.publish(gpio_stat, payload);
  }
  else if (!strcmp(topic, test_com))
  {
    client.publish(test_stat, "OSH Thermostat is Online!");
    client.publish(openhab_test, "ON");
    client.publish(version_stat, versionNum);
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    client.connect("ESP8266Thermo", mqtt_user, mqtt_pass);
    if (client.connected())
    {
      Serial.println("Connected");
      client.subscribe(gpio_com);
      client.loop();
      client.subscribe(test_com);
      client.loop();

      client.publish(allPub, "Thermostat just reconnected");
      client.publish(openhab_test, "ON");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println("Try again in 5 seconds...");
      if (WiFi.status() != WL_CONNECTED)
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

//PapI 2016