int leftBlink = 6;
int rightBlink = 4;
int headLight = 3;
int brakeLight = 2;
int brakeButton = 5;
int leftButton = 7;
int rightButton = 8;
int headButton = 9;

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
}

void loop()
{
  rightStateB = !digitalRead(rightButton);
  leftStateB = !digitalRead(leftButton);
  headStateB = !digitalRead(headButton);
  brakeStateB = digitalRead(brakeButton);

  if (rightStateB)
  {
    leftState = LOW;
    (rightState)?(rightState = LOW):(rightState = HIGH);
    delay(15);
    while (!digitalRead(rightButton))
    {
      delay(50);
    }
    lastTime = millis();
  }
  if (leftStateB)
  {
    rightState = LOW;
    (leftState)?(leftState = LOW):(leftState = HIGH);
    delay(15);
    while (!digitalRead(leftButton))
    {
      delay(50);
    }
    lastTime = millis();
  }
  if (headStateB)
  {
    if (headState)
    {
      headBrake("h", 0);
    }
    else
    {
      headBrake("b", 1);
    }
    delay(15);
    while (!digitalRead(headButton))
    {
      delay(50);
    }
  }
  if (brakeStateB)
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
  }
  if (rightState)
  {
    if (rightStateSub && ((millis() - lastTime) >= 1000))
    {
      digitalWrite(rightBlink, LOW);
      rightStateSub = LOW;
      lastTime = millis();
    }
    else if (!rightStateSub && ((millis() - lastTime) >= 1000))
    {
      digitalWrite(rightBlink, HIGH);
      rightStateSub = HIGH;
      lastTime = millis();
    }
  }
  else if (leftState)
  {
    if (leftStateSub && ((millis() - lastTime) >= 1000))
    {
      digitalWrite(leftBlink, LOW);
      leftStateSub = LOW;
      lastTime = millis();
    }
    else if (!leftStateSub && ((millis() - lastTime) >= 1000))
    {
      digitalWrite(leftBlink, HIGH);
      leftStateSub = HIGH;
      lastTime = millis();
    }
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
