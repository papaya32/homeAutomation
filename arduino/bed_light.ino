/*This code is designed for a single light and two
buttons for control. The first button will toggle
the light while the second will control nightMode.
By default, the nightMode turns on the light here
and issues a command, which so far turns the wall
light off, and will turn some specified desk lights
on.
Additionally, there is a status led, which is on
each of the buttons. This is dimmed using
the dimmer function when mqtt is disconnected,
blinks when wifi is disconnected, and toggles when
the light is on or off (opposite of light).
When nightMode is on and the nightMode button is
pressed again, it publishes to the allOff topic,
which effectively shuts off all the lights in the
bedroom, including the bed light.
Tested and working well now.
Version Notes:
1. Added FOTA support.
2. Added ability for adapting to different WiFi
networks.
//SIGNED//
JACK W. O'REILLY
20 July 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

String serial = "1954683";
String versionCode = "003";
String type = "14";
const char* versionNum = "2.00";
String versionTotal = type + ':' + serial + ':' + versionCode;
const char* ssid = serial.c_str();
//const char* ssid = "1954683";
const char* passphrase = "PapI2016";
String mqtt_user = "";
String mqtt_pass = "";
String st;
String content;
int statusCode;
unsigned long int lastTime = 0;

//void setup_wifi();  //wifi setup function initialization
void callback(char*, byte*, unsigned int);  //callback function for when one of the subscriptions gets a hit
void dimmer();  //dimmer functino for status led when mqtt is disconnected
void pushTest();  //function for testing when a button is pressed
void lightSwitch(bool);
bool testWiFi();
void launchWeb(int);
void setupAP();
void createWebServer(int);
void reset();
void resetTimer();

const char* mqtt_server = "mqtt.oreillyj.com";  //mqtt broker ip address or url (tcp)
int mqtt_port = 1883;

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
const char* version_stat = "osh/bed/bed1/version";

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
int resetPin = 5;

WiFiClient espClient;  //brings in wifi client
PubSubClient client(espClient);  //selects connectivity client for PubSubClient library
long lastMsg = 0;
char msg[50];
int value = 0;
int delayTime = 200;

void setup()
{
  EEPROM.begin(512);
  
  pinMode(relayPin, OUTPUT);  //relay pin as output
  pinMode(nightModePin, INPUT);
  pinMode(togglePin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(resetPin), reset, FALLING);
  
  Serial.begin(115200);
  Serial.println(versionTotal);
  Serial.print("OSH Bed Bed Light Pap Version: ");  //for my reference
  Serial.println(versionNum);
  
  //setup_wifi();  //calls for setting up wifi
  
  client.setServer(mqtt_server, mqtt_port);  //initializes connection to mqtt broker, port in used here
  client.setCallback(callback);  //sets callback function

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
      return true;
    }
    analogWrite(ledPin, 0);
    for (int i = 0; i < 50; i++)
    {
      delay(10);
      pushTest();
    }
    analogWrite(ledPin, 1023);
    for (int i = 0; i < 50; i++)
    {
      delay(10);
      pushTest();
    }
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
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(ledPin, LOW);
    delay(500);
    digitalWrite(ledPin, HIGH);
    delay(500);
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
  client.publish(version_stat, versionNum);
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
    client.connect(serial.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
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
    client.publish(version_stat, versionNum); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)  //if wifi is disconnected, probable cause of mqtt disconnect
      {
        testWiFi();
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
    tempStat = LOW;
    client.publish(night_stat, "OFF");
    client.publish(night_com, "0");
  client.publish(allOff, "ON");
    lastStateNight = LOW;
    lightSwitch(LOW);
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

//PapI 2016
