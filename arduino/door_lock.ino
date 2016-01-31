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
const char* allPub = "osh/all/stat";

bool lockState = LOW;

int servoPin = 14;
int ledPin = 16;
int doorSensor = 15;
int buttonUnlock = 12;
int buttonLock = 13;
int buttonWait = 5;
int accessRelay = 2;

int lockDegree = 150;
unsigned long referenceTime = 300;
unsigned long lastTime = 0;

Servo lockServo;

void lockDoor(int);
void accessVerify(unsigned long);
void dimmer();
void setup_wifi();
void callback(char*, byte*, unsigned int);

//Interrupt function initializations
void lock();
void unlock();
void wait();
void access();

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonLock, OUTPUT);
  pinMode(buttonUnlock, OUTPUT);
  pinMode(buttonWait, OUTPUT);
  pinMode(accessRelay, OUTPUT);
  pinMode(doorSensor, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(buttonLock), lock, RISING);
  attachInterrupt(digitalPinToInterrupt(buttonUnlock), unlock, RISING);
  attachInterrupt(digitalPinToInterrupt(buttonWait), wait, RISING);
  attachInterrupt(digitalPinToInterrupt(accessRelay), access, FALLING);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Door lock Pap Version 0.85");
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
    Serial.print(".");
    analogWrite(ledPin, 0);  //turn LED off
    delay(500);   
    analogWrite(ledPin, 1023);  //turn LED on
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  //prints local ip address
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)  //loop that prints out each character in char array that is message
  {
    Serial.print((char)payload[i]);  //prints out array element by element
  }
  Serial.println();
  
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
  }
}

void reconnect()  //this function is called repeatedly until mqtt is connected to broker
{
  while (!client.connected())  //while not connected...
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.connect("ESP8266Client");  //connect funtion, must include ESP8266Client
    if (client.connected())  //if connected...
    {
      client.subscribe(door_com);  //once connected, subscribe
      client.loop();
      client.subscribe(test_com);
      client.loop();
      client.publish(allPub, "Wall Light just reconnected!");
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
}

void lockDoor(int lockMode)  //door lock function
{
  lockServo.attach(servoPin);  //attaches to servo pin
  int i;
  delayMicroseconds(100000);
  if (lockMode)  //if parameter is a 1
  {
    for (i = 0; i <= lockDegree; i++)  //start for loop for each degree of turn
    {
      lockServo.write(i);  //write degree to servo
      Serial.println(i);
      delayMicroseconds(5000);  //delay so servo doesn't blow up
    }
    Serial.println("Door locked!");
  }
  else if (!lockMode)  //if 0 (unlock)
  {
    for (i = lockDegree; i >= 0; i--)
    {
      lockServo.write(i);
      Serial.println(i);
      delayMicroseconds(5000);
    }
    Serial.println("Door unlocked!");
  }
  lockServo.detach();  //detaches so the servo isn't super stiff and the key can be turned (just in case)
}

void dimmer()  //dimmer function (for recognizing disconnect when in wall)
{
  int i;
  int pos;
  for (i = 0; i < 1023; i++) //loop from dark to bright in ~3.1 seconds
  {
    analogWrite(ledPin, i);  //set PWM rate
    delay(3);  //ideal timing for dimming rate
  }
  for (i = 1023; i > 0; i--) //slowly turn back off
  {
    analogWrite(ledPin, i);
    delay(3);
  }
}

void lock()
{
  lockDoor(1);
}

void unlock()
{
  lockDoor(0);
}

void wait()
{
  //Wait shit
  Serial.println("Something's working");
}

void access()
{
  unsigned long currentTime = millis();
  accessVerify(currentTime);
}

void accessVerify(unsigned long currentTime)
{
  if ((currentTime - lastTime) <= referenceTime)
  {
    lockDoor(0);
  }
  lastTime = currentTime;
  delayMicroseconds(20000);
}