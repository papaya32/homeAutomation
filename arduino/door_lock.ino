/*This is the code that will go on the front
door to control the deadbolt. It will consist
of three buttons on the deadbolt itself, and
control wires going to the relay on the access
controller. It has specific times the relay
is supposed to be open, to prevent people from
ripping off the controller and unlocking the door.
There will also be controls in openHAB to control
the deadbolt via mqtt. 

VERSION NOTES:

-There is no code for the wait button. It will
be added later.

-The code has not been tested with either the
servo nor the access controller, so tweaks
may have to be made.

-The buttons have also not technically been
tested, but the code was copied from working
versions so this should be ok.

-Good comments need to be added.
//SIGNED//
JACK W. O'REILLY
31 Jan 2016*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>  //mqtt client library
#include <Servo.h>
//#include <SPI.h>
//#include <SD.h>

//File root;

// Update these with values suitable for your network.

const char* ssid = "oreilly";  //wifi ssid
const char* password = "9232616cc8";  //wifi password
const char* mqtt_server = "192.168.1.108";  //server IP (port later defined)

const char* door_com = "osh/liv/door/com";
const char* test_com = "osh/all/test/com";

const char* door_stat = "osh/liv/door/stat";
const char* test_stat = "osh/all/test/stat";
const char* openhab_stat = "osh/liv/door/openhab";
const char* doorbell_stat = "osh/liv/door/doorBell";
const char* allPub = "osh/all/stat";

bool lockState = LOW;

int servoPin = 14;
int ledPin = 15;
int doorSensor = 16;

#define buttonUnlock 12
#define buttonLock 13
#define buttonWait 4
#define accessRelay 2
#define doorBellButt 5

bool currentStateButton = LOW;
bool lastStateButton = LOW;

#define numButtons 5
char* buttonArray[numButtons] = {"12", "13", "5", "2", "5"};

int lockDegree = 150;
int counter = 0;
unsigned long referenceTime = 260;

Servo lockServo;

void lockDoor(int);
void dimmer();
void setup_wifi();
void callback(char*, byte*, unsigned int);
void buttonPress();
void doorBellFunc();

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//void printDirectory(File, int);

void setup()
{
  //Serial.begin(9600);

  //Serial.print("Initializing SD card...");
  
  pinMode(ledPin, OUTPUT);
  pinMode(buttonLock, INPUT_PULLUP);
  pinMode(buttonUnlock, INPUT_PULLUP);
  pinMode(buttonWait, INPUT_PULLUP);
  pinMode(accessRelay, INPUT_PULLUP);
  pinMode(doorSensor, INPUT_PULLUP);

    // Open serial communications and wait for port to open:

  /*if (!SD.begin(9)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");*/
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Door lock Pap Version 0.9");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  //starts wifi connection

  while (WiFi.status() != WL_CONNECTED) {  //while wifi not connected...
    int i;
    Serial.print(".");
    analogWrite(ledPin, 0);  //turn LED off
    for (i = 0; i <= 50; i++)
    {
       buttonPress();
       delay(10);
    }
    analogWrite(ledPin, 1023);  //turn LED on
    for (i = 0; i <= 50; i++)
    {
       buttonPress();
       delay(10);
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  //prints local ip address
}

void callback(char* topic, byte* payload, unsigned int length)
{

  if (((char)payload[0] == '1') && !strcmp(topic, door_com))
  {
    lockDoor(1);
    client.publish(door_stat, "ON");
    client.publish(allPub, "Living Room Door is Locked");
  }
  else if (((char)payload[0] == '0') && strcmp(topic, door_com))
  {
    lockDoor(0);
    client.publish(door_stat, "OFF");
    client.publish(allPub, "Living Room Door is Unlocked");
  }
  else if (((char)payload[0] == '1') && strcmp(topic, test_com))
  {
    client.publish(test_stat, "Living Room Door Controller is Online!");
    client.publish(openhab_stat, "ON");
  }
}

void reconnect()  //this function is called repeatedly until mqtt is connected to broker
{
  while (!client.connected())  //while not connected...
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESPClientDo");  //connect funtion, must include ESP8266Client
    if (client.connected())  //if connected...
    {
      client.subscribe(door_com);  //once connected, subscribe
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.publish(allPub, "Wall Light just reconnected!");
    client.publish(openhab_stat, "ON");
    }
    else  //if we're not connected
    {
      Serial.print("failed, rc=");  //if failed, prints out the reason (look up rc codes in api header file)
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      if (WiFi.status() != WL_CONNECTED)  //if wifi is disconnected (could be reason for mqtt not connecting)
      {
        setup_wifi();  //calls wifi setup function
      }
      dimmer();  //dim the button LED on and off (around 5 second delay)
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  buttonPress();
  yield();
}

void lockDoor(int lockMode)  //door lock function
{
  int i;
  if (lockMode)  //if parameter is a 1
  {
  lockServo.attach(servoPin);
    lockServo.write(lockDegree);
  delay(3000);
  lockServo.detach();
  }
  else if (!lockMode)  //if 0 (unlock)
  {
    lockServo.attach(servoPin);
    lockServo.write(0);
    delay(3000);
    lockServo.detach();
  }  
}

void dimmer()  //dimmer function (for recognizing disconnect when in wall)
{
  int i;
  int pos;
  for (i = 0; i < 1023; i++) //loop from dark to bright in ~3.1 seconds
  {
    analogWrite(ledPin, i);  //set PWM rate
    buttonPress();
    delay(3);  //ideal timing for dimming rate
    yield();
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    buttonPress();
    delay(3);
    yield();
  }
}

void buttonPress()  //function that
{
  int i;
  for (i = 0; i < numButtons; i++)  //for loop that checks each button
  {
    int currentButton = atoi(buttonArray[i]);
    currentStateButton = digitalRead(currentButton);  //current state is reading the state of the button
    if (!currentStateButton)  //if the button is currently being pressed...
    {
      switch (currentButton)  //switch statement where the argument is the pin number that is currently being pushed
      {
        case buttonLock:  //if it's the lock button...
          lockDoor(1);  //lock the door
          client.publish(door_stat, "ON");  //publish that the door's locked
          break;
        case buttonUnlock:  //if unlock button..
          lockDoor(0);
          client.publish(door_stat, "OFF");
          break;
        case accessRelay:
          counter = 0;
          while (!digitalRead(accessRelay))
          {
            delay(5);
            yield();
            counter += 5;
          }
          if ((counter >= (referenceTime - .1 * referenceTime)) || (counter <= (referenceTime + .1 * referenceTime)))
          {
            lockDoor(0);
          }
          break;
        case buttonWait:
          int i = 1;
          while (i <= 5)
          {
            if (!digitalRead(doorSensor))
            {
              lockDoor(1);
              i = 6;
            }
            i++;
          }
          break;
        /*case doorBellButt:
          doorBellFunc();
          break;*/
       }
      while (!digitalRead(currentButton))  //while the current button is still being pressed...
      {
        yield();  //delay so the esp doesn't blow up/crash :)
      }
    }
  yield();
  }
}

void doorBellFunc()
{
  client.publish(doorbell_stat, "ON");
  yield();
}

/*void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}*/
