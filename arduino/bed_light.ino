#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //pasword
const char* mqtt_server = "192.168.1.108";  //mqtt broker ip address or url (tcp)

const char* bed1_com = "osh/bed/bed1/com";  //command mqtt for bedside light
const char* night_com = "osh/bed/nightMode/com";  //command mqtt for night mode
const char* temp_com = "osh/bed/all/temp";  //command for temporarily switching lights off then on if previously on
const char* test_com = "osh/all/test/com";  //command for testing which esp's are online

const char* bed1_stat = "osh/bed/bed1/stat";  //stat mqtt for publishing status of bedside light
const char* night_stat = "osh/bed/nightMode/stat";  //publish status of night mode for when button is pushed
const char* test_stat = "osh/all/test/stat";  //place to publish replies to test command

bool bed1Stat = LOW;  //initializes status of light to off
bool nightStat;  //status of nightmode
bool currentStateBed = LOW;  //current state of bed button
bool lastStateBed = LOW;  //last state of bed button (for toggling)
bool currentStateNight = LOW;  //see above :)
bool lastStateBed = LOW;

int relayPin = 14;  //pin for controlling relay which controls light
int nightModePin = 16;  //pin for night mode button
int togglePin = 12;  //pin for light switch toggle
int ledPin = 15;  //pin for status led

WiFiClient espClient;  //brings in wifi client
PubSubClient client(espClient);  //selects connectivity client for PubSubClient library
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi();  //wifi setup function initialization
void callback(char*, byte*, unsigned int);  //callback function for when one of the subscriptions gets a hit
void dimmer();  //dimmer functino for status led when mqtt is disconnected
void pushTest();  //function for testing when a button is pressed
void lightOff();  //turns the light off
void lightOn();  //vice versa

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin as output
  pinMode(nightModePin, INPUT);
  pinMode(togglePin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("OSH Bed Bed Light Pap Version 0.8");  //for my reference
  setup_wifi();  //calls for setting up wifi
  client.setServer(mqtt_server, 1883);  //initializes connection to mqtt broker, port in used here
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
    delay(250);  //wait a quarter of a second
    analogWrite(ledPin, 1023);  //turn led on
    delay(250);
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
    lightOn();  //turn light on
  }
  else if (((char)payload[0] == '0') && (strcmp(topic, bed1_com) == 0))  //bed light gets off command
  {
    lightOff();
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, temp_com))  //if temp com topic gets "off" command
  {
    digitalWrite(relayPin, LOW);  //turn off the light
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, temp_com))  //if temp mode turns back on
  {
    digitalWrite(relayPin, bed1Stat);  //switch light to whatever the last status was
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))  //if test topic was called
  {
    client.publish(test_stat, "OSH Bed Bed Light is Online!");  //publish esp status
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, night_com))  //if night mode is turned on
  {
    if (bed1Stat)  //if light is on
    {
      lightOff();  //turn light off
      bed1Stat = HIGH;  //set back to high because lightOff() turns it to low (for night mode turning off later)
    }
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, night_com))  //if night mode is turned off
  {
    if (bed1Stat)  //if previous status was HIGH
    {
      lightOn();  //turn it back on
    }
  }
}

void reconnect()
{
  while (!client.connected())  //while mqtt is not connected
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266Client");
    if (client.connected()) {
      Serial.println("connected");
      client.subscribe(bed1_com);  //subscribe to necessary topics
      client.loop();  //bug in PubSubClient library code causes mqtt to fail with multiple subscriptions
      //client.loop() call after each subscription is a workaround
      client.subscribe(test_com);
      client.loop();
      client.subscribe(temp_com);
      client.loop();
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
    }
  }
}
void loop() {

  if (!client.connected())  //if mqtt is disconnected
  {
    reconnect();  //reconnect funtion
  }
  client.loop();
  pushTest();  //tests for button presses
}

void pushTest()
{
  currentStateBed = digitalRead(togglePin);  //updates current state of light switch button
  currentStateNight = digitalRead(nightPin);  //updates current state of night mode button
  if (currentStateBed && !lastStateBed)  //if button is being pushed and was previously off
  {
    lightOn();
    lastStateBed = HIGH;  //update last button state for next time button is pressed
    while (digitalRead(togglePin))  //while button is being pressed...
    {
      delay(5);  //delay so esp doesn't crash
    }
  }
  else if (currentStateBed && lastStateBed)  //if currently pushed and previously was on
  {
    lightOff();
    lastStateBed = LOW;
    while (digitalRead(togglePin))
    {
      delay(5);
    }
  }
  if (currentStateNight && !lastStateNight)  //if button is being pushed and was previously off
  {
    client.publish(night_stat, "ON");  //publish status to status topic
    client.publish(night_com, "1");  //publish command for other esp's
    lastStateNight = HIGH;  //update state
    while (digitalRead(nightPin))  //while button is bing pressed
    {
      delay(5);
    }
  }
  else if (currentStateNight && lastStateNight)  //if currently pushed and previously was on
  {
    client.publish(night_stat, "OFF");
    client.publish(night_com, "0");
    lastStateNight = LOW;
    while (digitalRead(nightPin))
    {
      delay(5);
    }
  }
}

void dimmer()
{
  int i;
  for (i = 0; i < 1023; i++) //loop from dark to bright in ~3.1 seconds
  {
    analogWrite(ledPin, i);  //set PWM rate
    pushTest();  //call button press function (so switch can be used when offline)
    delay(3);  //ideal timing for dimming rate
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    pushTest();
    delay(3);
  }
}

void lightOn()
{
  digitalWrite(relayPin, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  bed1Stat = HIGH;
  client.publish(bed1_stat, "ON");
  analogWrite(ledPin, 0);
}

void lightOff()
{
  digitalWrite(relayPin, LOW);   // Turn the LED on (Note that LOW is the voltage level
  bed1Stat = LOW;
  client.publish(bed1_stat, "OFF");
  analogWrite(ledPin, 1023);
}
