/* This is hopefully the last update that
fixes night mode problems and some simple
wrong topic issues. Looking good.

Added FOTA and WiFi Changes

//SIGNED//
JACK W. O'REILLY
20 July 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

String serial = "1954973";
String versionCode = "002";
String type = "13";
const char* versionNum = "1.90";
String versionTotal = type + ':' + serial + ':' + versionCode;
const char* ssid = serial.c_str();
const char* passphrase = "PapI2016";
String mqtt_user = "";
String mqtt_pass = "";
String st;
String content;
int statusCode;
unsigned long int lastTime = 0;

const char* mqtt_server = "mqtt.oreillyj.com";
int mqtt_port = 1883;  //changed from previously 1884 as a contingency plan

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
int resetPin = 14;

void lightSwitch(int, bool);  //called whenever a relay needs to be flipped, arguments are relay number and state to flip to
void publishStats();  //function that simply publishes all of the relay stats (desk1 on, desk2 off, etc)
void tempFunc(int);  //called when temp button is pressed, either turns all off or previously on on
//void setup_wifi();
void callback(char*, byte*, unsigned int);
bool testWiFi();
void launchWeb(int);
void setupAP();
void createWebServer(int);
void reset();
void resetTimer();

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  EEPROM.begin(512);
  
  pinMode(desk1Pin, OUTPUT);
  pinMode(desk2Pin, OUTPUT);
  pinMode(desk3Pin, OUTPUT);
  pinMode(desk4Pin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(resetPin), reset, FALLING);

  lightSwitch(1, LOW);  //initialize all relays by flipping off
  lightSwitch(2, LOW);
  lightSwitch(3, LOW);
  lightSwitch(4, LOW);
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Desk Light Pap Version: ");  //for my reference in debugging
  Serial.println(versionNum);
  //setup_wifi();  //connect to wifi
  client.setServer(mqtt_server, mqtt_port);  //initialize mqtt server connection
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
  while (c < 20)
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
      return true;
    }
    delay(500);
    c++;
  }
  return false;
}

void launchWeb (int webtype)
{
  Serial.println();
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
      content += "<h1>Success!</h1><p>Using username: ";
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
    client.connect(serial.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());  //connect with unique identifier
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
        //setup_wifi();
        testWiFi();
      }
      delay(5000);
    }
  }
}

void loop()
{
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
  if (!client.connected() && (WiFi.status() == WL_CONNECTED))
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
