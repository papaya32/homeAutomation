/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic55" every two seconds
  - subscribes to the topic "inTopic55", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic55" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "oreilly";
const char* password = "9232616cc8";
const char* mqtt_server = "192.168.1.108";

const char* bed1_com = "osh/bed/bed1/com";
const char* temp_com = "osh/bed/all/temp";
const char* test_com = "osh/all/test/com";

const char* bed1_stat = "osh/bed/bed1/stat";
const char* test_stat = "osh/all/test/stat";

bool bed1Stat = LOW;

int relayPin = 14;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi();
void callback(char*, byte*, unsigned int);

void setup() {
  pinMode(relayPin, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  Serial.println("OSH Bed Bed Light Pap Version 0.8");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (((char)payload[0] == '1') && (strcmp(topic, bed1_com) == 0))
  {
    digitalWrite(relayPin, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    bed1Stat = HIGH;
    client.publish(bed1_stat, "ON");
  }
  else if (((char)payload[0] == '0') && (strcmp(topic, bed1_com) == 0))
  {
    digitalWrite(relayPin, LOW);  // Turn the LED off by making the voltage HIGH
    bed1Stat = LOW;
    client.publish(bed1_stat, "OFF");
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, temp_com))
  {
    digitalWrite(relayPin, LOW);
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, temp_com))
  {
    digitalWrite(relayPin, bed1Stat);
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))
  {
    client.publish(test_stat, "OSH Bed Bed Light is Online!");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266Client");
    if (client.connected()) {
      Serial.println("connected");
      client.subscribe(bed1_com);
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.subscribe(temp_com);
      client.loop();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)
      {
        setup_wifi();
      }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
