/*This code is designed for a single light and two
buttons for control. The first button will toggle
the light while the second will control nightMode.
By default, the nightMode turns off the light here
and issues a command, which so far only affects the
desk lights.

Additionally, there is a status led, which likely
will be on both the buttons. This is dimmed using
the dimmer function when mqtt is disconnected,
blinks when wifi is disconnected, and toggles when
the light is on or off (opposite of light).

When nightMode is on and the nightMode button is
pressed again, it publishes to the allOff topic,
which effectively shuts off all the lights in the
bedroom.

Compiles and deployment will be tested shortly.

//SIGNED//
JACK W. O'REILLY
23 Mar 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //pasword
const char* mqtt_server = "mqtt.oreillyj.com";  //mqtt broker ip address or url (tcp)
int mqtt_port = 1884;
const char* mqtt_user = "bed1";
const char* mqtt_pass = "24518000bed1";

const char* bed1_com = "osh/bed/bed1/com";  //command mqtt for bedside light
const char* night_com = "osh/bed/nightMode/com";  //command mqtt for night mode
const char* temp_com = "osh/bed/all/temp";  //command for temporarily switching lights off then on if previously on
const char* test_com = "osh/all/test/com";  //command for testing which esp's are online
const char* openhab_start = "osh/all/start";

const char* bed1_stat = "osh/bed/bed1/stat";  //stat mqtt for publishing status of bedside light
const char* night_stat = "osh/bed/nightMode/stat";  //publish status of night mode for when button is pushed
const char* test_stat = "osh/all/test/stat";  //place to publish replies to test command
const char* openhab_reconnect = "osh/bed/bed1/reconnect";
const char* openhab_test = "osh/bed/bed1/openhab";
const char* allOff = "osh/bed/all/allOff";

bool nightStat;  //status of nightmode
bool currentStateBed = LOW;  //current state of bed button
bool bed1Stat = LOW;  //last state of bed button (for toggling)
bool currentStateNight = LOW;  //see above :)
bool lastStateNight = LOW;
bool tempStat = LOW;

int relayPin = 14;  //pin for controlling relay which controls light
int nightModePin = 12;  //pin for night mode button
int togglePin = 4;  //pin for light switch toggle
int ledPin = 16;  //pin for status led

WiFiClient espClient;  //brings in wifi client
PubSubClient client(espClient);  //selects connectivity client for PubSubClient library
long lastMsg = 0;
char msg[50];
int value = 0;
int delayTime = 200;

void setup_wifi();  //wifi setup function initialization
void callback(char*, byte*, unsigned int);  //callback function for when one of the subscriptions gets a hit
void dimmer();  //dimmer functino for status led when mqtt is disconnected
void pushTest();  //function for testing when a button is pressed
void lightSwitch(bool);

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin as output
  pinMode(nightModePin, INPUT);
  pinMode(togglePin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("OSH Bed Bed Light Pap Version 0.8");  //for my reference
  setup_wifi();  //calls for setting up wifi
  client.setServer(mqtt_server, mqtt_port);  //initializes connection to mqtt broker, port in used here
  client.setCallback(callback);  //sets callback function
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)  //if not connected...
  {
    analogWrite(ledPin, 0);  //turn led off
    delay(500);  //wait half of a second
    analogWrite(ledPin, 1023);  //turn led on
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
  if (((char)payload[0] == '1') && (strcmp(topic, bed1_com) == 0))  //if bed light gets on command
  {
    lightSwitch(HIGH);  //turn light on
  }
  else if (((char)payload[0] == '0') && (strcmp(topic, bed1_com) == 0))  //bed light gets off command
  {
    lightSwitch(LOW);
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, temp_com))  //if temp com topic gets "off" command
  {
    tempStat = bed1Stat;
  lightSwitch(LOW);
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, temp_com))  //if temp mode turns back on
  {
    lightSwitch(tempStat);  //switch light to whatever the last status was
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //if test topic was called
  {
    client.publish(test_stat, "OSH Bed Bed Light is Online!");  //publish esp status
  client.publish(openhab_test, "ON");
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, night_com))  //if night mode is turned on
  {
  tempStat = bed1Stat;
    lightSwitch(HIGH);
    lastStateNight = HIGH;
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, night_com))  //if night mode is turned off
  {
    lightSwitch(tempStat);
    lastStateNight = LOW;
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, openhab_start))
  {
   if (nightStat)
   {
      client.publish(night_com, "ON");
   }
   if (!nightStat)
   {
      client.publish(night_com, "OFF");
   }
   if (bed1Stat)
   {
      client.publish(bed1_stat, "ON");
   }
   if (!bed1Stat)
   {
      client.publish(bed1_stat, "OFF");
   }
  }
}

void reconnect()
{
  while (!client.connected())  //while mqtt is not connected
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266ClientBed", mqtt_user, mqtt_pass);
    if (client.connected()) {
      Serial.println("connected");
      client.subscribe(bed1_com);  //subscribe to necessary topics
      client.loop();  //bug in PubSubClient library code causes mqtt to fail with multiple subscriptions
      //client.loop() call after each subscription is a workaround
      client.subscribe(test_com);
      client.loop();
      client.subscribe(temp_com);
      client.loop();
    client.subscribe(night_com);
    client.loop();
    client.subscribe(openhab_start);
    client.loop();
    
    client.publish(openhab_reconnect, "ON");    
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)  //if wifi is disconnected, probable cause of mqtt disconnect
      {
        setup_wifi();
      }
      // Wait 5 seconds before retrying
      dimmer();  //dims and relights the led ot indicate to user that mqtt is not connected, takes 6.2 sec
    if (!bed1Stat)
    {
      analogWrite(ledPin, 1023);
    }
    }
  }
}
void loop() {

  if (!client.connected())  //if mqtt is disconnected
  {
    reconnect();  //reconnect funtion
  }
  client.loop();
  yield();
  pushTest();  //tests for button presses
}

void pushTest()
{
  currentStateBed = digitalRead(togglePin);  //updates current state of light switch button
  currentStateNight = digitalRead(nightModePin);  //updates current state of night mode button
  if (currentStateBed && !bed1Stat)  //if button is being pushed and was previously off
  {
    lightSwitch(HIGH);
    while (digitalRead(togglePin))  //while button is being pressed...
    {
      delay(5);  //delay so esp doesn't crash
    yield();
    }
    delay(delayTime);
  }
  else if (currentStateBed && bed1Stat)  //if currently pushed and previously was on
  {
    lightSwitch(LOW);
    while (digitalRead(togglePin))
    {
      delay(5);
    yield();
    }
    delay(delayTime);
  }
  if (currentStateNight && !lastStateNight)  //if button is being pushed and was previously off
  {
    client.publish(night_stat, "ON");  //publish status to status topic
    client.publish(night_com, "1");  //publish command for other esp's
    lastStateNight = HIGH;  //update state
    while (digitalRead(nightModePin))  //while button is bing pressed
    {
      delay(5);
    yield();
    }
    delay(delayTime);
  }
  else if (currentStateNight && lastStateNight)  //if currently pushed and previously was on
  {
    client.publish(night_stat, "OFF");
    client.publish(night_com, "0");
  client.publish(allOff, "ON");
    lastStateNight = LOW;
    while (digitalRead(nightModePin))
    {
      delay(5);
    yield();
    }
    delay(delayTime);
  }
}

void dimmer()
{
  int i;
  for (i = 0; i < 1023; i++) //loop from dark to bright in ~3.1 seconds
  {
    analogWrite(ledPin, i);  //set PWM rate
    pushTest();  //call button press function (so switch can be used when offline)
  yield();
    delay(3);  //ideal timing for dimming rate
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    pushTest();
  yield();
    delay(3);
  }
}

void lightSwitch(bool state)
{
  if (state)
  {
    digitalWrite(relayPin, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    bed1Stat = HIGH;
    client.publish(bed1_stat, "ON");
    analogWrite(ledPin, 0);
  }
  else
  {
    digitalWrite(relayPin, LOW);   // Turn the LED on (Note that LOW is the voltage level
    bed1Stat = LOW;
    client.publish(bed1_stat, "OFF");
    analogWrite(ledPin, 1023);
  }
}
