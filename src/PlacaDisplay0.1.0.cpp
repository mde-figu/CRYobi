#include <Arduino.h>
#include <Wire.h>

const byte I2C_ADDRESS = 0x08; // Single I2C address for the board até 0x11

bool disp7seg[11][7] = {
  {1,1,1,1,1,1,0}, //0
  {0,1,1,0,0,0,0}, //1
  {1,1,0,1,1,0,1}, //2
  {1,1,1,1,0,0,1}, //3
  {0,1,1,0,0,1,1}, //4
  {1,0,1,1,0,1,1}, //5
  {1,0,1,1,1,1,1}, //6
  {1,1,1,0,0,0,0}, //7
  {1,1,1,1,1,1,1}, //8
  {1,1,1,1,0,1,1}, //9
  {0,0,0,0,0,0,0}  //blank
};

const int pins7seg[] = {2, 3, 4, 5, 6, 7, 8};
const int increaseRate = 250;
const int debounceTime = 50;
const int blinkDuration = 1000; // Blink for 1 second after value change
const int blinkInterval = 250;  // 250ms on/off cycle for value change
const int lockBlinkInterval = 100; // 100ms on/off cycle for locked state
const int lockPressDuration = 2000; // 2 seconds to lock/unlock

#define ButtonUpCh1 A3
#define ButtonDwCh1 A2
#define ButtonUpCh2 A1
#define ButtonDwCh2 A0
#define UpGeneral A6
#define DwGeneral A7
#define delayBlockUnblock 1000
#define CtrdigMSB_Ch1 10
#define CtrdigLSB_Ch1 11
#define CtrdigMSB_Ch2 12
#define CtrdigLSB_Ch2 13

int valueCh1 = 0; // Current value for Channel 1 (0-99)
int valueCh2 = 0; // Current value for Channel 2 (0-99)
unsigned long lastButtonTimeCh1 = 0;
unsigned long lastButtonTimeCh2 = 0;
unsigned long lastChangeTimeCh1 = 0; // Time of last Ch1 value change
unsigned long lastChangeTimeCh2 = 0; // Time of last Ch2 value change
bool displayOnCh1 = true; // Blink state for Ch1
bool displayOnCh2 = true; // Blink state for Ch2
bool blinkActiveCh1 = false; // Whether Ch1 is blinking due to value change
bool blinkActiveCh2 = false; // Whether Ch2 is blinking due to value change
unsigned long lastBlinkTimeCh1 = 0; // Last blink toggle for Ch1
unsigned long lastBlinkTimeCh2 = 0; // Last blink toggle for Ch2
bool lockedCh1 = false; // Lock state for Ch1
bool lockedCh2 = false; // Lock state for Ch2
unsigned long lockPressStartCh1 = 0; // Start time of Ch1 lock press
unsigned long lockPressStartCh2 = 0; // Start time of Ch2 lock press
bool lockPressActiveCh1 = false; // Whether Ch1 lock press is being detected
bool lockPressActiveCh2 = false; // Whether Ch2 lock press is being detected

// Function prototypes
void receiveEvent(int howMany);
void requestEvent();
void updateDisplay(int digit);
void handleButtons();
void updateBlinkState();
void updateDisplayMultiplex();
void sendI2C(byte channel, int value);

void setup() {
  // Initialize Serial communication
  Serial.begin(9600);
  while (!Serial); // Wait for Serial to initialize
  
  // Initialize 7-segment pins
  for (int i = 0; i < 7; i++) {
    pinMode(pins7seg[i], OUTPUT);
  }
  
  // Initialize digit control pins
  pinMode(CtrdigMSB_Ch1, OUTPUT);
  pinMode(CtrdigLSB_Ch1, OUTPUT);
  pinMode(CtrdigMSB_Ch2, OUTPUT);
  pinMode(CtrdigLSB_Ch2, OUTPUT);
  
  // Initialize button pins
  pinMode(ButtonUpCh1, INPUT_PULLUP);
  pinMode(ButtonDwCh1, INPUT_PULLUP);
  pinMode(ButtonUpCh2, INPUT_PULLUP);
  pinMode(ButtonDwCh2, INPUT_PULLUP);
  
  // Initialize I2C
  Wire.begin(I2C_ADDRESS); // Join I2C bus with single address
  Wire.onReceive(receiveEvent); // Register receive event
  Wire.onRequest(requestEvent); // Register request event
  
  // Test all digits to diagnose display issues
  digitalWrite(CtrdigMSB_Ch1, HIGH);
  digitalWrite(CtrdigLSB_Ch1, LOW);
  digitalWrite(CtrdigMSB_Ch2, LOW);
  digitalWrite(CtrdigLSB_Ch2, LOW);
  updateDisplay(8); // Show '8' on Ch1 MSB
  delay(1000);
  digitalWrite(CtrdigMSB_Ch1, LOW);
  digitalWrite(CtrdigLSB_Ch1, HIGH);
  updateDisplay(8); // Show '8' on Ch1 LSB
  delay(1000);
  digitalWrite(CtrdigLSB_Ch1, LOW);
  digitalWrite(CtrdigMSB_Ch2, HIGH);
  updateDisplay(8); // Show '8' on Ch2 MSB
  delay(1000);
  digitalWrite(CtrdigMSB_Ch2, LOW);
  digitalWrite(CtrdigLSB_Ch2, HIGH);
  updateDisplay(8); // Show '8' on Ch2 LSB
  delay(1000);
  digitalWrite(CtrdigLSB_Ch2, LOW);
  updateDisplay(0); // Clear display
  
  Serial.println("Setup complete");
}

void loop() {
  handleButtons();
  updateBlinkState();
  updateDisplayMultiplex();
}

// Handle button presses with debouncing
void handleButtons() {
  unsigned long currentTime = millis();
  
  // Channel 1 buttons
  if (currentTime - lastButtonTimeCh1 > debounceTime) {
    bool upPressedCh1 = digitalRead(ButtonUpCh1) == LOW;
    bool downPressedCh1 = digitalRead(ButtonDwCh1) == LOW;
    
    // Check for simultaneous up and down press for locking/unlocking
    if (upPressedCh1 && downPressedCh1) {
      if (!lockPressActiveCh1) {
        lockPressActiveCh1 = true;
        lockPressStartCh1 = currentTime;
      } else if (currentTime - lockPressStartCh1 >= lockPressDuration) {
        lockedCh1 = !lockedCh1;
        lockPressActiveCh1 = false;
        lastButtonTimeCh1 = currentTime;
        Serial.print("Ch1 Lock: ");
        Serial.print(lockedCh1 ? "Locked" : "Unlocked");
        Serial.print(" at ");
        Serial.print(currentTime);
        Serial.println("ms");
      }
    } else {
      lockPressActiveCh1 = false; // Reset if both buttons aren't pressed
      if (!lockedCh1) {
        if (upPressedCh1) {
          valueCh1 = min(99, valueCh1 + 1);
          lastButtonTimeCh1 = currentTime;
          lastChangeTimeCh1 = currentTime;
          blinkActiveCh1 = true;
          sendI2C(1, valueCh1);
        } else if (downPressedCh1) {
          valueCh1 = max(0, valueCh1 - 1);
          lastButtonTimeCh1 = currentTime;
          lastChangeTimeCh1 = currentTime;
          blinkActiveCh1 = true;
          sendI2C(1, valueCh1);
        }
      }
    }
  }
  
  // Channel 2 buttons
  if (currentTime - lastButtonTimeCh2 > debounceTime) {
    bool upPressedCh2 = digitalRead(ButtonUpCh2) == LOW;
    bool downPressedCh2 = digitalRead(ButtonDwCh2) == LOW;
    
    // Check for simultaneous up and down press for locking/unlocking
    if (upPressedCh2 && downPressedCh2) {
      if (!lockPressActiveCh2) {
        lockPressActiveCh2 = true;
        lockPressStartCh2 = currentTime;
      } else if (currentTime - lockPressStartCh2 >= lockPressDuration) {
        lockedCh2 = !lockedCh2;
        lockPressActiveCh2 = false;
        lastButtonTimeCh2 = currentTime;
        Serial.print("Ch2 Lock: ");
        Serial.print(lockedCh2 ? "Locked" : "Unlocked");
        Serial.print(" at ");
        Serial.print(currentTime);
        Serial.println("ms");
      }
    } else {
      lockPressActiveCh2 = false; // Reset if both buttons aren't pressed
      if (!lockedCh2) {
        if (upPressedCh2) {
          valueCh2 = min(99, valueCh2 + 1);
          lastButtonTimeCh2 = currentTime;
          lastChangeTimeCh2 = currentTime;
          blinkActiveCh2 = true;
          sendI2C(2, valueCh2);
        } else if (downPressedCh2) {
          valueCh2 = max(0, valueCh2 - 1);
          lastButtonTimeCh2 = currentTime;
          lastChangeTimeCh2 = currentTime;
          blinkActiveCh2 = true;
          sendI2C(2, valueCh2);
        }
      }
    }
  }
}

// Update blink states for each channel
void updateBlinkState() {
  unsigned long currentTime = millis();
  
  // Channel 1 blink control
  if (blinkActiveCh1) {
    if (currentTime - lastChangeTimeCh1 >= blinkDuration) {
      blinkActiveCh1 = false;
      displayOnCh1 = true; // Stay on after blinking
    } else if (currentTime - lastBlinkTimeCh1 >= blinkInterval) {
      displayOnCh1 = !displayOnCh1;
      lastBlinkTimeCh1 = currentTime;
    }
  } else if (lockedCh1 && (currentTime - lastBlinkTimeCh1 >= lockBlinkInterval)) {
    displayOnCh1 = !displayOnCh1;
    lastBlinkTimeCh1 = currentTime;
  }
  
  // Channel 2 blink control
  if (blinkActiveCh2) {
    if (currentTime - lastChangeTimeCh2 >= blinkDuration) {
      blinkActiveCh2 = false;
      displayOnCh2 = true; // Stay on after blinking
    } else if (currentTime - lastBlinkTimeCh2 >= blinkInterval) {
      displayOnCh2 = !displayOnCh2;
      lastBlinkTimeCh2 = currentTime;
    }
  } else if (lockedCh2 && (currentTime - lastBlinkTimeCh2 >= lockBlinkInterval)) {
    displayOnCh2 = !displayOnCh2;
    lastBlinkTimeCh2 = currentTime;
  }
}

// Send value over I2C with channel identifier
void sendI2C(byte channel, int value) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(channel); // Send channel identifier (1 for Ch1, 2 for Ch2)
  Wire.write((byte)(value & 0xFF)); // Send value
  Wire.endTransmission();
  
  // Log outgoing payload
  Serial.print("I2C Out: Address=0x");
  Serial.print(I2C_ADDRESS, HEX);
  Serial.print(", Channel=");
  Serial.print(channel);
  Serial.print(", Value=");
  Serial.println(value);
}

// I2C receive event
void receiveEvent(int howMany) {
  Serial.print("I2C Received: Address=0x");
  Serial.print(I2C_ADDRESS, HEX);
  Serial.print(", Bytes=");
  Serial.println(howMany);

  if (howMany >= 3) { // Expect [type, channel, value]
    byte type = Wire.read(); // 0 for target, 1 for actual
    byte channel = Wire.read();
    int receivedValue = Wire.read();

    Serial.print("Type=");
    Serial.print(type);
    Serial.print(", Channel=");
    Serial.print(channel);
    Serial.print(", Value=");
    Serial.println(receivedValue);

    if (type == 1) { // Only update actuals (potentiometer values)
      if (channel == 1 && !lockedCh1) {
        valueCh1 = constrain(receivedValue, 0, 99);
        lastChangeTimeCh1 = millis();
        blinkActiveCh1 = true;
        Serial.print("Updated Ch1 to ");
        Serial.println(valueCh1);
      } else if (channel == 2 && !lockedCh2) {
        valueCh2 = constrain(receivedValue, 0, 99);
        lastChangeTimeCh2 = millis();
        blinkActiveCh2 = true;
        Serial.print("Updated Ch2 to ");
        Serial.println(valueCh2);
      }
    }
  }
  while (Wire.available()) {
    byte discarded = Wire.read();
    Serial.print("Discarded byte: 0x");
    Serial.println(discarded, HEX);
  }
}

// I2C request event
void requestEvent() {
  // Respond with both channels' values
  Wire.write((byte)1); // Channel 1 identifier
  Wire.write((byte)(valueCh1 & 0xFF)); // Channel 1 value
  Wire.write((byte)2); // Channel 2 identifier
  Wire.write((byte)(valueCh2 & 0xFF)); // Channel 2 value
  
  // Log request response payload
  Serial.print("I2C Request Out: Channel=1, Value=");
  Serial.print(valueCh1);
  Serial.print("; Channel=2, Value=");
  Serial.println(valueCh2);
}

// Update 7-segment display with multiplexing
void updateDisplayMultiplex() {
  // Ensure all digits are off before setting new digit
  digitalWrite(CtrdigMSB_Ch1, LOW);
  digitalWrite(CtrdigLSB_Ch1, LOW);
  digitalWrite(CtrdigMSB_Ch2, LOW);
  digitalWrite(CtrdigLSB_Ch2, LOW);
  
  // Display Channel 1 MSB (tens digit)
  digitalWrite(CtrdigMSB_Ch1, displayOnCh1 ? HIGH : LOW);
  updateDisplay(valueCh1 / 10);
  delay(5);
  digitalWrite(CtrdigMSB_Ch1, LOW);
  
  // Display Channel 1 LSB (units digit)
  digitalWrite(CtrdigLSB_Ch1, displayOnCh1 ? HIGH : LOW);
  updateDisplay(valueCh1 % 10);
  delay(5);
  digitalWrite(CtrdigLSB_Ch1, LOW);
  
  // Display Channel 2 MSB (tens digit)
  digitalWrite(CtrdigMSB_Ch2, displayOnCh2 ? HIGH : LOW);
  updateDisplay(valueCh2 / 10);
  delay(5);
  digitalWrite(CtrdigMSB_Ch2, LOW);
  
  // Display Channel 2 LSB (units digit)
  digitalWrite(CtrdigLSB_Ch2, displayOnCh2 ? HIGH : LOW);
  updateDisplay(valueCh2 % 10);
  delay(5);
  digitalWrite(CtrdigLSB_Ch2, LOW);
}

// Update 7-segment display with a single digit
void updateDisplay(int digit) {
  for (int i = 0; i < 7; i++) {
    digitalWrite(pins7seg[i], disp7seg[digit][i]);
  }
}