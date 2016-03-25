/*This is the code that will be living on the coffee maker.
It has a few key functions. In terms of interfaces with the
coffee maker, it is currently limited. It has a relay which
simulates pressing the on/off button on the board. Additionally
it has a button with an LED on the side. The primary function
for this button is to indicate when the machine has been
prepped. When this happens, the LED will light up, indicating
it's prepped, and tell the home automation software. If you
double press the button (currently set to 1 second in the
home automation software) it will toggle the coffee maker.
This is, however, handled entirely outside of the machine.

Version Notes:

1. We need to interface the machine's board status LED with
the ESP to more accurately keep the coffee maker's state.

2. When connected to MQTT, with the latest update the version
will print out to a topic, for my own reference so I know
which version of the code is loaded. My lack of organization
has proven the necessity of this.

//SIGNED//
JACK W. O'REILLY
25 Mar 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "mqtt.oreillyj.com";  //server IP (port later defined)
int mqtt_port = 1884;
const char* mqtt_user = "coffee";
const char* mqtt_pass = "24518000coffee";

const char* coffee_com = "osh/kit/coffee/com";  //command mqtt topic
const char* test_com = "osh/all/test/com";  //command mqtt for live testing
const char* openhab_start = "osh/all/start";  //command for when openhab boots and needs to update statuses

const char* coffee_stat = "osh/kit/coffee/stat";  //status mqtt topic
const char* coffee_prep = "osh/kit/coffee/prep";  //status for prep button
const char* test_stat = "osh/all/test/stat";  //status mqtt for live testing
const char* openhab_test = "osh/kit/coffee/openhab";  //for openhab testing if esp online
const char* openhab_reconnect = "osh/kit/coffee/reconnect";  //to publish to openhab when mqtt reconnects
const char* allPub = "osh/all/stat";
const char* version_stat = "osh/kit/coffee/version";

const char* versionNum = "1.11";

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

void setup()
{
  pinMode(relayPin, OUTPUT);  //relay pin
  digitalWrite(relayPin, HIGH);  //so the coffee maker doesn't turn on on reset
  
  pinMode(prepButton, INPUT);  //button
  pinMode(onPin, INPUT);  //connects to coffee maker "on" pin
  pinMode(ledPin, OUTPUT);  //connects to led on coffee board for status indication
  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.print("Coffee Switch Pap Version: ");  //for my reference
  Serial.println(versionNum);
  setup_wifi();  //starts up wifi (user defined function)
  client.setServer(mqtt_server, mqtt_port);  //connects to mqtt server (second parameter is port)
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
    delay(500);
    analogWrite(ledPin, 0);
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
    coffeeState = digitalRead(onPin);  //read if the coffee maker is on or not
    if (!coffeeState && prepStat)  //if the coffee maker is off AND the prep button has been pushed
    {
      coffeeSwitch();  //"push the button"
      client.publish(coffee_stat, "ON");
    }
    else if (!prepStat)  //if prep button hasn't been pressed
    {
      client.publish(allPub, "Prep button has not been pressed");  //informational message
    }
  }
  else if (((char)payload[0] == '0') && !strcmp(topic, coffee_com))
  {
    //coffeeState = digitalRead(onPin);  //read the status of the coffee board led lead
    if (coffeeState)  //if it's on..
    {
      coffeeSwitch();  //flip the switch
      client.publish(coffee_stat, "OFF");
    }
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, test_com))
  {
    client.publish(test_stat, "Coffee Controller is online!");
    client.publish(openhab_test, "ON");
    client.publish(version_stat, versionNum);
  }
  else if (((char)payload[0] == '1') && !strcmp(topic, openhab_start))
  {
    if (coffeeState)
  {
      client.publish(coffee_stat, "ON");
  }
  if (!coffeeState)
  {
    client.publish(coffee_stat, "OFF");
  }
  if (prepStat)
  {
    client.publish(coffee_prep, "ON");
  }
  if (!prepStat)
  {
    client.publish(coffee_prep, "OFF");
  }
  }
}

void reconnect() {  //this function is called repeatedly until mqtt is connected to broker
  while (!client.connected())  //while not connected...
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266ClientCoffee", mqtt_user, mqtt_pass);  //connect funtion, must include ESP8266Client
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
    client.subscribe(openhab_start);
    client.loop();
    
    client.publish(openhab_reconnect, "ON");
      
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
  if (!prepStat)
  {
    analogWrite(ledPin, 0);
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
  yield();
}

void coffeeSwitch()
{
  digitalWrite(relayPin, LOW);  //click the relay on
  delay(250);  //wait a quarter of a second
  digitalWrite(relayPin, HIGH);  //turn the relay off
  
  bool tempCoff = coffeeState;
  coffeeState = !tempCoff;
  analogWrite(ledPin, 0);  //turn on the indicator led
  prepStat = LOW;
}

void pushTest()  //function to test for button press (for prepping coffee maker)
{
  bool buttonState = digitalRead(prepButton);
  if (buttonState)
  {
    prepStat = HIGH;
    while (digitalRead(prepButton))  //while button is being pressed
    {
      yield();  //keeps esp from crashing
    }
    coffeeState = digitalRead(onPin);  //updates status of coffee maker to what the onPin says (just in case)
    client.publish(allPub, "Coffee maker prepped");  //update the status reader
    client.publish(coffee_prep, "ON");
    analogWrite(ledPin, 1023);
  }
  coffeeState = digitalRead(onPin);
  //the purpose of the next two ifs is to keep from writing to pins every millisecond, will only do so if the status changes
  if (coffeeState && !lastState)  //if coffee maker is on and was just recently off..
  {
    analogWrite(ledPin, 0);  //turn on led
    lastState = HIGH;  //updates the previous state
  }
  else if (!coffeeState && lastState)  //otherwise (if coffee maker of off)
  {
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
