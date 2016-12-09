int leftBlink = 6;
int rightBlink = 10;
int headLight = 9;
int brakeLight = 2;
int brakeButton = 11;
int leftButton = 3;
int rightButton = 5;
int headButton = 4;

bool rightState = LOW;
bool leftState = LOW;
bool rightStateSub = LOW;
bool leftStateSub = LOW;
bool headState = LOW;
bool brakeState = LOW;

bool rightStateB = LOW;
bool leftStateB = LOW;
bool headStateB = LOW;
bool brakeStateB = LOW;

unsigned long int lastTime = millis();
int resTime = 500;

void headBrake(char*, int);

void setup()
{
  pinMode(leftBlink, OUTPUT);
  pinMode(rightBlink, OUTPUT);
  pinMode(headLight, OUTPUT);
  pinMode(brakeLight, OUTPUT);
  pinMode(brakeButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(headButton, INPUT);

  digitalWrite(brakeButton, HIGH);
  digitalWrite(leftButton, HIGH);
  digitalWrite(rightButton, HIGH);
  digitalWrite(headButton, HIGH);

  Serial.begin(115200);
  Serial.println("BEGIN");
}

void loop()
{
  rightStateB = !digitalRead(rightButton);
  leftStateB = !digitalRead(leftButton);
  headStateB = !digitalRead(headButton);
  brakeStateB = digitalRead(brakeButton);

  //Serial.println(digitalRead(headButton));

  if (rightStateB)
  {
    Serial.println("RIGHT BUTTON");
    leftState = LOW;
    (rightState) ? (rightState = LOW) : (rightState = HIGH);
    delay(15);
    while (!digitalRead(rightButton))
    {
      delay(50);
    }
    lastTime = millis();
  }
  if (leftStateB)
  {
    Serial.println("LEFT BUTTON");
    rightState = LOW;
    (leftState) ? (leftState = LOW) : (leftState = HIGH);
    delay(15);
    while (!digitalRead(leftButton))
    {
      delay(50);
    }
    lastTime = millis();
  }
  if (headStateB)
  {
    Serial.println("HEAD BUTTON");
    if (headState)
    {
      digitalWrite(headLight, LOW);
      headState = LOW;
      Serial.println("HEADLIGHT IS NOW OFF");
      //headBrake("h", 0);
      int counter = 0;
      while (!digitalRead(headButton))
      {
        delay(10);
        counter += 10;
      }
      if (counter <= resTime)
      {
        Serial.println("ONLY SWITCHED");
        digitalWrite(headLight, HIGH);
        headState = HIGH;
      }
    }
    else
    {
      digitalWrite(headLight, HIGH);
      headState = HIGH;
      //headBrake("b", 1);
    }
    delay(15);
    while (!digitalRead(headButton))
    {
      delay(50);
    }
  }
  /*if (brakeStateB)
    {
    if (brakeState)
    {
      headBrake("b", 0);
    }
    else
    {
      headBrake("b", 1);
    }
    delay(15);
    while (digitalRead(brakeButton))
    {
      delay(50);
    }
    }*/
  if (rightState)
  {
    if (rightStateSub && ((millis() - lastTime) >= 750))
    {
      Serial.println("RIGH OFF");
      digitalWrite(rightBlink, LOW);
      rightStateSub = LOW;
      lastTime = millis();
    }
    else if (!rightStateSub && ((millis() - lastTime) >= 750))
    {
      Serial.println("RIGHT ON");
      digitalWrite(rightBlink, HIGH);
      rightStateSub = HIGH;
      lastTime = millis();
    }
  }
  else if (leftState)
  {
    if (leftStateSub && ((millis() - lastTime) >= 750))
    {
      Serial.println("LEFT OFF");
      digitalWrite(leftBlink, LOW);
      leftStateSub = LOW;
      lastTime = millis();
    }
    else if (!leftStateSub && ((millis() - lastTime) >= 750))
    {
      Serial.println("LEFT ON");
      digitalWrite(leftBlink, HIGH);
      leftStateSub = HIGH;
      lastTime = millis();
    }
  }
  else
  {
    digitalWrite(leftBlink, HIGH);
    digitalWrite(rightBlink, HIGH);
  }
}

void headBrake(char* device, int newState)
{
  if (device == "h")
  {
    if (newState)
    {
      digitalWrite(headLight, HIGH);
      headState = HIGH;
    }
    else
    {
      digitalWrite(headLight, LOW);
      headState = LOW;
    }
    Serial.println("HEAD IS: " + headState);
  }
  else if (device == "b")
  {
    if (newState)
    {
      digitalWrite(brakeLight, HIGH);
      brakeState = HIGH;
    }
    else
    {
      digitalWrite(brakeLight, LOW);
      brakeState = LOW;
    }
  }
}
