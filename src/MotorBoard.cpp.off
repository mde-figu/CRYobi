#include <Arduino.h>
#include <Wire.h>

// I2C Configuration
#define I2C_ADDRESS 0x08 // Configurable: 0x08 to 0x12 for each MotorBoard

// Motor and LED Pins (Inverse Direction)
const int ledCW = 6;    // Channel 1: Clockwise
const int ledOp = 4;    // Channel 1: Operation (HIGH = stopped)
const int ledCCW = 9;   // Channel 1: Counter-Clockwise
const int ledCWA = 5;   // Channel 2: Clockwise
const int ledOpA = 7;   // Channel 2: Operation (HIGH = stopped)
const int ledCCWA = 3;  // Channel 2: Counter-Clockwise
const int ledPROT = 8;  // Protection LED

// Analog Inputs
const int potPinCh1 = A1;   // Channel 1 potentiometer
const int potPinCh2 = A0;   // Channel 2 potentiometer
const int sensePin = A6;    // Motor lock sense

// Motor Control Variables
const int minPot = 10;      // Minimum potentiometer value (not used)
const int maxPot = 700;     // Maximum potentiometer value (not used)
int valorPot;               // Channel 1 potentiometer (0-1023)
int valorPotA;              // Channel 2 potentiometer (0-1023)
int valorSet = 0;           // Channel 1 setpoint (0-255)
int valorSetA = 0;          // Channel 2 setpoint (0-255)
int valorSense;             // Sense value (0-1023)
byte potValueCh1;           // Scaled Ch1 pot for I2C (0-255)
byte potValueCh2;           // Scaled Ch2 pot for I2C (0-255)
bool travou = false;        // Channel 1 lock flag
bool travouA = false;       // Channel 2 lock flag
bool enableLockDetection = false; // Toggle lock detection (set to true to enable)

// I2C Buffer
byte i2cResponse[4] = {1, 0, 2, 0}; // [1, potValueCh1, 2, potValueCh2]

// Function Prototypes
void requestEvent();
void receiveEvent(int numBytes);

void setup() {
  // Initialize Serial
  Serial.begin(9600);
  while (!Serial);

  // Initialize I2C as Slave
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(requestEvent); // Handle SlaveBoard requests
  Wire.onReceive(receiveEvent); // Handle SlaveBoard writes

  // Verify I2C initialization
  Serial.print("MotorBoard Started at I2C=0x");
  Serial.println(I2C_ADDRESS, HEX);
  Serial.println("I2C Slave Initialized, waiting for events...");

  // Initialize Pins
  pinMode(ledCW, OUTPUT);
  pinMode(ledOp, OUTPUT);
  pinMode(ledCCW, OUTPUT);
  pinMode(ledCWA, OUTPUT);
  pinMode(ledOpA, OUTPUT);
  pinMode(ledCCWA, OUTPUT);
  pinMode(ledPROT, OUTPUT);

  // Initial States
  digitalWrite(ledOp, HIGH);   // Stopped
  digitalWrite(ledCW, HIGH);   // Off
  digitalWrite(ledCCW, HIGH);  // Off
  digitalWrite(ledOpA, HIGH);  // Stopped
  digitalWrite(ledCWA, HIGH);  // Off
  digitalWrite(ledCCWA, HIGH); // Off
  digitalWrite(ledPROT, LOW);  // Protection off
}

void loop() {
  // Read Potentiometers
  valorPot = analogRead(potPinCh1);   // 0-1023
  valorPotA = analogRead(potPinCh2);  // 0-1023
  potValueCh1 = map(valorPot, 0, 1023, 0, 255); // Scale to 0-255 for I2C
  potValueCh2 = map(valorPotA, 0, 1023, 0, 255); // Scale to 0-255 for I2C

  // Update I2C Response Buffer
  i2cResponse[1] = potValueCh1;
  i2cResponse[3] = potValueCh2;

  // Motor Lock Detection (Optional)
  if (enableLockDetection) {
    valorSense = analogRead(sensePin);
    if (valorSense > 99) {
      Serial.print("Bloqueio motor, valor sense: ");
      Serial.println(valorSense);
      if (digitalRead(ledOp) == HIGH) {
        travou = true;
      }
      if (digitalRead(ledOpA) == HIGH) {
        travouA = true;
      }
      digitalWrite(ledOp, LOW);
      digitalWrite(ledOpA, LOW);
      digitalWrite(ledCW, HIGH);
      digitalWrite(ledCCW, HIGH);
      digitalWrite(ledCWA, HIGH);
      digitalWrite(ledCCWA, HIGH);
      digitalWrite(ledPROT, LOW);
      delay(100);
      return; // Skip motor control if locked
    }
  }

  // Channel 1 Motor Control
  int valorSetMaior = valorSet * 4 + 5; // Scale setpoint to 0-1023 range
  int valorSetMenor = valorSet * 4 - 5;
  Serial.print("Ch1: valorSet=");
  Serial.print(valorSet);
  Serial.print(", valorPot=");
  Serial.print(valorPot);
  Serial.print(", potValueCh1=");
  Serial.println(potValueCh1);

  if (valorPot >= valorSetMenor && valorPot <= valorSetMaior) {
    digitalWrite(ledOp, HIGH);  // Stop
    digitalWrite(ledCW, HIGH);  // Off
    digitalWrite(ledCCW, HIGH); // Off
  } else {
    if (valorPot > valorSetMaior) {
      digitalWrite(ledOp, LOW);   // Running
      digitalWrite(ledCW, HIGH);  // Off
      digitalWrite(ledCCW, LOW);  // CCW
    } else if (valorPot < valorSetMenor) {
      digitalWrite(ledOp, LOW);   // Running
      digitalWrite(ledCW, LOW);   // CW
      digitalWrite(ledCCW, HIGH); // Off
    }
  }

  // Channel 2 Motor Control
  int valorSetMaiorA = valorSetA * 4 + 5; // Scale setpoint to 0-1023 range
  int valorSetMenorA = valorSetA * 4 - 5;
  Serial.print("Ch2: valorSetA=");
  Serial.print(valorSetA);
  Serial.print(", valorPotA=");
  Serial.print(valorPotA);
  Serial.print(", potValueCh2=");
  Serial.println(potValueCh2);

  if (valorPotA >= valorSetMenorA && valorPotA <= valorSetMaiorA) {
    digitalWrite(ledOpA, HIGH);   // Stop
    digitalWrite(ledCWA, HIGH);   // Off
    digitalWrite(ledCCWA, HIGH);  // Off
  } else {
    if (valorPotA > valorSetMaiorA) {
      digitalWrite(ledOpA, LOW);    // Running
      digitalWrite(ledCWA, HIGH);   // Off
      digitalWrite(ledCCWA, LOW);   // CCW
    } else if (valorPotA < valorSetMenorA) {
      digitalWrite(ledOpA, LOW);    // Running
      digitalWrite(ledCWA, LOW);    // CW
      digitalWrite(ledCCWA, HIGH);  // Off
    }
  }

  delay(10); // Small delay for stability
}

// I2C Request Handler: Send [1, potValueCh1, 2, potValueCh2]
void requestEvent() {
  Serial.println("I2C Request received");
  Wire.write(i2cResponse, 4);
  Serial.print("I2C Sent: MotorBoard=0x");
  Serial.print(I2C_ADDRESS, HEX);
  Serial.print(", Data=[1,");
  Serial.print(i2cResponse[1]);
  Serial.print(",2,");
  Serial.print(i2cResponse[3]);
  Serial.println("]");
}

// I2C Receive Handler: Receive [channel, value]
void receiveEvent(int numBytes) {
  Serial.print("I2C Receive event triggered, numBytes=");
  Serial.println(numBytes);
  if (numBytes == 2) {
    byte channel = Wire.read();
    byte value = Wire.read();
    if (channel == 1) {
      valorSet = value; // Update Channel 1 setpoint
      Serial.print("I2C Received: MotorBoard=0x");
      Serial.print(I2C_ADDRESS, HEX);
      Serial.print(", Ch=1, Val=");
      Serial.println(value);
    } else if (channel == 2) {
      valorSetA = value; // Update Channel 2 setpoint
      Serial.print("I2C Received: MotorBoard=0x");
      Serial.print(I2C_ADDRESS, HEX);
      Serial.print(", Ch=2, Val=");
      Serial.println(value);
    } else {
      Serial.print("I2C Error: MotorBoard=0x");
      Serial.print(I2C_ADDRESS, HEX);
      Serial.print(", Invalid channel=");
      Serial.println(channel);
    }
  } else {
    Serial.print("I2C Error: MotorBoard=0x");
    Serial.print(I2C_ADDRESS, HEX);
    Serial.print(", Invalid bytes=");
    Serial.println(numBytes);
    while (Wire.available()) {
      Serial.print("Discarded byte: 0x");
      Serial.println(Wire.read(), HEX);
    }
  }
}