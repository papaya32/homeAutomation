#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* coffee_com = "osh/kit/coffee/com";  //command mqtt topic
const char* test_com = "osh/all/test/com";  //command mqtt for live testing

const char* coffee_stat = "osh/kit/coffee/stat";  //status mqtt topic
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing

bool coffeeState = LOW;  //variable for testing if coffee maker on or off

int relayPin = 14;
int ledPin = 16;
int buttonPin = 4;

void pushTest();
void coffeeSwitch();

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
  pinMode(ledPin, INPUT);
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println("Coffee Switch Pap Version 0.75");  //for my reference
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
    delay(500);
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
  if (((char)payload[0] == '1') && !strcmp(topic, coffee_com))  //if message is one and topics match...
  {
    coffeeState = digitalRead(ledPin);  //turn on coffee maker
    if (!coffeeState)
    {
      coffeeSwitch();
    }
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, coffee_com))
  {
    coffeeState = digitalRead(ledPin);
    if (coffeeState)
    {
      coffeeSwitch();
    }
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))
  {
    client.publish(test_stat, "Coffee Controller is online!");
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
      client.subscribe(coffee_com);  //once connected, subscribe
      client.subscribe(test_com);
    }
    else
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}
void loop() {

  if (!client.connected())  //if not connected to mqtt...
  {
    reconnect();  //call reconnect function
  }
  client.loop();  //keeps searching subscriptions, absolutely required
  pushTest();
}

void coffeeSwitch()
{
  digitalWrite(relayPin, LOW);
  delay(250);
  digitalWrite(relayPin, HIGH);
}

void pushTest()  //function to test for button press
{
  bool buttonState = digitalRead(buttonPin);
  if (buttonState)
  {
    coffeeSwitch();
    while (buttonState)
    {
      buttonState = digitalRead(buttonPin);
      delay(5);
    }
    coffeeState = digitalRead(ledPin);
    if (coffeeState)
    {
      client.publish(coffee_stat, "ON");
    }
    else
    {
      client.publish(coffee_stat, "OFF");
    }
  }
}
