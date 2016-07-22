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
3. This is a possible deliverable as we have implemented server
code that allows an end user to utilize their own wifi network
and my mqtt service.
4. We have implemented Firmware Over The Air, using a server
running PHP code to evaluate whether or not an ESP needs an
update, and if so, which binary to deliver. It's pretty kickass.
//SIGNED//
JACK W. O'REILLY
19 July 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

String serial = "4681973";
String versionCode = "019";
String type = "16";
String versionTotal = type + ':' + serial + ':' + versionCode;
//const char* ssid = serial.c_str();
const char* ssid = "4681973";
const char* passphrase = "PapI2016";
String st;
String content;
int statusCode;
unsigned long int lastTime = 0;

bool testWiFi();
void launchWeb(int);
void setupAP();
void createWebServer(int);
void reset();
void resetTimer();

// Update these with values suitable for your network.

const char* mqtt_server = "mqtt.oreillyj.com";  //server IP (port later defined)
int mqtt_port = 1883;
String mqtt_user = "";
String mqtt_pass = "";

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

const char* versionNum = "2.01";

bool coffeeState = LOW;  //variable for testing if coffee maker on or off
bool prepStat = LOW;  //state of prep (HIGH if ready to be turned on); only applies to commands over mqtt
bool lastState = LOW;  //boolean variable to keep from updating led status each loop (see pushTest function)

int relayPin = 14;  //pin controlling relay to switch the coffee maker on and off
int ledPin = 16;  //status led pin, connected to actual led built in on board
int prepButton = 4;  //input button pin to push when the coffee maker has coffee and water in it
int onPin = 13;  //connected to coffee maker's board where the on indicator (built into board) lives
int resetPin = 5;

void pushTest();  //function for testing for button presses
void coffeeSwitch();  //simple function to flip relay and update on/off status
void dimmer();  //function to dim and then power back up status led (when mqtt is connecting)

WiFiClient espClient;  //part of base code for library use
PubSubClient client(espClient);  //specifies esp as the AP connector
long lastMsg = 0;
char msg[50];  //initializes char array for mqtt message
int value = 0;  //utility variable

void callback(char*, byte*, unsigned int);  //callback function for when subscribed topic gets a message

void setup()
{
  pinMode(relayPin, OUTPUT);  //relay pin
  digitalWrite(relayPin, HIGH);  //so the coffee maker doesn't turn on on reset

  EEPROM.begin(512);
  
  pinMode(prepButton, INPUT);  //button
  pinMode(onPin, INPUT);  //connects to coffee maker "on" pin
  pinMode(ledPin, OUTPUT);  //connects to led on coffee board for status indication
  pinMode(resetPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(resetPin), reset, FALLING);

  Serial.begin(115200);  //initializes baud rate for serial monitor
  Serial.print("Coffee Switch Pap Version: ");  //for my reference
  Serial.println(versionNum);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  String esid = "";
  String epass = "";

  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  if (esid.length() > 1)
  {
    WiFi.begin(esid.c_str(), epass.c_str());
    WiFi.mode(WIFI_STA);
    if (testWiFi())
    {
      launchWeb(0);
      return;
    }
  }
  setupAP();
}

bool testWiFi(void)
{
  int c = 0;
  Serial.println("TESTING WIFI");
  while (c < 10)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      mqtt_user = "";
      mqtt_pass = "";
      for (int i = 0; i < 32; ++i)
      {
        mqtt_user += char(EEPROM.read(i + 96));
        mqtt_pass += char(EEPROM.read(i + 128));
      }
      analogWrite(ledPin, 0);
      return true;
    }
    analogWrite(ledPin, 1023);
    delay(500);
    analogWrite(ledPin, 0);
    delay(500);
    c++;
  }
  return false;
}

void launchWeb (int webtype)
{
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(2, LOW);
    delay(250);
    digitalWrite(2, HIGH);
    delay(250);
  }
  int n = WiFi.scanNetworks();
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    st += "<li>";
    st += WiFi.SSID(i);
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  launchWeb(1);
}

void createWebServer(int webtype)
{
  if (webtype == 1)
  {
    server.on("/", []()
    {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String (ip[0]) + '.' + String (ip[1]) + '.' + String (ip[2]) + '.' + String (ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>PapI 2016";
      content += "<p>";
      content += st;
      content += "</p><form method='post' action='apset'><lable>SSID: </label><input name='ssid' length=32><input type='password' name='pass' lengthe=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/userset", []()
    {
      String user_name = server.arg("user_name");
      String password = server.arg("password");
      Serial.print("Username: ");
      Serial.println(user_name);
      Serial.print("Pass: ");
      Serial.println(password);
      mqtt_user = const_cast<char*>(user_name.c_str());
      mqtt_pass = const_cast<char*>(password.c_str());
      if  (user_name.length() > 0 && password.length() > 0)
      {
        for (int i = 0; i < user_name.length(); ++i)
        {
          EEPROM.write(i + 96, user_name[i]);
        }
        for (int i = 0; i < password.length(); ++i)
        {
          EEPROM.write(i + 128, password[i]);
        }
        EEPROM.commit();
      }
      content = "<!DOCTYPE HTML>\r\n<html>PapI 2016";
      content += "<h1>Success!</h1><p>Using username";
      content += user_name;
      content += ".</p><p>Resetting in 5 seconds...</p></html>";
      server.send(200, "text/html", content);
      delay(5000);
      ESP.restart();
    });
    server.on("/apset", []()
    {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0)
      {
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
        }
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
        }
        EEPROM.commit();
        content = "<!DOCTYPE HTML>\r\n<html>PapI 2016";
        content += "<h1>Success!</h1><br><p>Connecting to ";
        content += qsid;
        content += " upon reset.</p><br><h3>Now enter your PapI credentials:</h3><br><form method='post' action='userset'><label>Username: </label><input name='user_name' length=32><input type='password' name='password' length=32><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);
     }
     else
     {
      content = "{\"Error\":\"404 not found\"}";
      statusCode=404;
      Serial.println("Sending 404");
     }
     server.send(statusCode, "application/json", content);
    });
  }
  else if (webtype == 0)
  {
    server.on("/", []()
    {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []()
    {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      for (int i = 0; i < 96; ++i)
      {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
    });
  }
}

void reset()
{
  resetTimer();
}

void resetTimer()
{
  Serial.println("Interrupted");
  int counter = 0;
  while (!digitalRead(resetPin))
  {
    delayMicroseconds(100000);
    counter++;
    Serial.println(counter);
  }
  if ((counter * 100) >= 5000)
  {
    Serial.println("Resetting");
    for (int i = 1; i < 160; ++i)
    {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    Serial.println("CLEARED");
    ESP.restart();
  }
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
    Serial.println();
    Serial.println();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect(serial.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());  //connect funtion, must include ESP8266Client
    //client.connect("ESP8266ClientCoffee", "tester", "tester123");
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
        testWiFi();  //reconnect to wifi, probable cause of mqtt disconnect
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

  server.handleClient();
  if (((millis() - lastTime) > 60 * 10 * 1000) || (lastTime == 0))
  {
    t_httpUpdate_return ret = ESPhttpUpdate.update("update.oreillyj.com", 80, "/downTest/updater.php", versionTotal);
    switch (ret)
    {
      case HTTP_UPDATE_FAILED:
        Serial.println("[Update] Update failed");
        lastTime = millis();
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[Update] No Updates");
        lastTime = millis();
        break;
      case HTTP_UPDATE_OK:
        Serial.println("[Update] Update OK");
        break;
    }
  }
  if (!client.connected() && (WiFi.status() == WL_CONNECTED))  //if not connected to mqtt...
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
