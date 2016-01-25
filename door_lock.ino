#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <Servo.h>

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* door_com = "osh/liv/door/com";
const char* test_com = "osh/all/test/com";

const char* door_stat = "osh/liv/door/stat";
const char* test_stat = "osh/all/test/stat";

bool currentStateLock = LOW;
bool lastStateLock = LOW;
bool currentStateUnlock = LOW;
bool lastStateUnlock = LOW;
bool currentStateWait = LOW;
bool lastStateWait = LOW;

int servoPin = 14;
int ledPin = 16;
int buttonLock = 4;
int buttonUnlock = 12;
int buttonWait = 13;

int locked = 150;
int unlocked = 160;

