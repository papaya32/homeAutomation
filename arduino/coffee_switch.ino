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
bool prepStat = LOW;  //state of prep (HIGH if ready to be turned on); only applies to commands over mqtt
bool lastState = LOW;  //boolean variable to keep from updating led status each loop (see pushTest function)

int relayPin = 14;  //pin controlling relay to switch the coffee maker on and off
int ledPin = 16;  //status led pin, connected to actual led built in on board
int prepButton = 4;  //input button pin to push when the coffee maker has coffee and water in it
int onPin = 13;  //connected to coffee maker's board where the on indicator (built into board) lives

void pushTest();  //function for testing for button presses
void coffeeSwitch();  //simple function to flip relay and update on/off status
void dimmer();  //function to dim and then power back up status led (when mqtt is connecting)

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void setup_wifi();  //initializes wifi setup function
void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

void setup() {
  pinMode(relayPin, OUTPUT);  //relay pin
  pinMode(prepButton, INPUT);  //button
  pinMode(onPin, INPUT);  //connects to coffee maker "on" pin
  pinMode(ledPin, OUTPUT);  //connects to led on coffee board for status indication
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.println("Coffee Switch Pap Version 0.8");  //for my reference
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
    analogWrite(ledPin, 1023);
    delay(250);
    analogWrite(ledPin, 0);
    delay(250);
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
    coffeeState = digitalRead(onPin);  //read if the coffee maker is on or not
    if (!coffeeState && prepStat)  //if the coffee maker is off AND the prep button has been pushed
    {
      coffeeSwitch();  //"push the button"
      client.publish(coffee_stat, "ON");
    }
    else if (!prepStat)  //if prep button hasn't been pressed
    {
      client.publish(coffee_stat, "Prep button has not been pressed");  //informational message
    }
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, coffee_com))
  {
    coffeeState = digitalRead(onPin);  //read the status of the coffee board led lead
    if (coffeeState)  //if it's on..
    {
      coffeeSwitch();  //flip the switch
    }
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))
  {
    client.publish(test_stat, "Coffee Controller is online!");
  }
  else if (((char)payload[0] == '2') && !strcmp(topic, coffee_com))  //special case for status request
  {
    coffeeState = digitalRead(onPin);
    client.publish(coffee_stat, coffeeState);  //publishes status of coffee maker
    if (prepStat)  //if the coffee maker is prepped
    {
      client.publish(coffee_stat, "Coffee maker is prepped");  //publish readable message
    }
    else  //otherwise (coffee maker is not prepped)
    {
      client.publish(coffee_stat, "Coffee maker is not prepped");  //publish readable message
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
      client.subscribe(coffee_com);  //once connected, subscribe
      client.loop();  //workaround for error in library causing mqtt reconnect issue with multiple subscriptions
      client.subscribe(test_com);
      client.loop();
      
      if (WiFi.status() != WL_CONNECTED)  //if the wifi is disconnected
      {
        setup_wifi();  //reconnect to wifi, probable cause of mqtt disconnect
      }
    }
    else  //if mqtt connection failed
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      dimmer(); //dims led to indicate status, takes about 6.2 sec
    }
  }
}
void loop() {

  if (!client.connected())  //if not connected to mqtt...
  {
    reconnect();  //call reconnect function
  }
  client.loop();  //keeps searching subscriptions, absolutely required
  pushTest();  //tests for prep button press
}

void coffeeSwitch()
{
  digitalWrite(relayPin, LOW);  //click the relay on
  delay(250);  //wait a quarter of a second
  digitalWrite(relayPin, HIGH);  //turn the relay off
  
  coffeeState = digitalRead(onPin);  //update status to test if coffee maker is on or off
  if (coffeeState)  //if the coffee maker is on..
  {
    analogWrite(ledPin, 1023);  //turn on the indicator led
  }
  else  //otherwise..
  {
    analogWrite(ledPin, 0);  //turn it off
  }
}

void pushTest()  //function to test for button press (for prepping coffee maker)
{
  bool buttonState = digitalRead(prepButton);
  if (buttonState)
  {
    prepStat = HIGH;
    while (buttonState)  //while button is being pressed
    {
      buttonState = digitalRead(prepButton);  //update status
      delay(5);  //keeps esp from crashing
    }
    coffeeState = digitalRead(onPin);  //updates status of coffee maker to what the onPin says (just in case)
    client.publish(coffee_stat, "Coffee maker prepped");  //update the status reader
  }
  coffeeState = digitalRead(onPin);
  //the purpose of the next two ifs is to keep from writing to pins every millisecond, will only do so if the status changes
  if (coffeeState && !lastState)  //if coffee maker is on and was just recently off..
  {
    analogWrite(ledPin, 1023);  //turn on led
    lastState = HIGH;  //updates the previous state
  }
  else if (!coffeeState && lastState)  //otherwise (if coffee maker of off)
  {
    analogWrite(ledPin, 0);  //turn led off
    lastState = LOW;  //updates the previous state
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
