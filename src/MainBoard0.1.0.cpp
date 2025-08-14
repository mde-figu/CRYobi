// MainBoard Code
#include <Arduino.h>
#include <Wire.h>

const int DE_PIN = 2; // RS485 DE pin
const byte START_BYTE = 0xAA;
const byte END_BYTE = 0xBB;
const byte DISPLAY_BASE_ADDR = 0x08; // DisplayBoards 0x08 to 0x12
const byte NUM_DISPLAY_BOARDS = 11;
const byte NUM_CHANNELS = 22; // 11 * 2
const unsigned long POLL_INTERVAL = 200; // ms
const byte CASTLE_UP_PIN = A0; // Button to increase castle
const byte CASTLE_DOWN_PIN = A1; // Button to decrease castle
const int DEBOUNCE_TIME = 50;

byte current_castle = 1; // Start with Castle 1 (1-4)
byte targets[22];
byte actuals[22];
unsigned long last_poll = 0;
unsigned long last_button_time = 0;
bool initial_setup_done = false;
void handleCastleSwitch();
void switchCastle(byte castle);
void pollTargets();
void pollActualsAndUpdate();
void sendToDisplays(byte type, byte* values);
bool getPotsFromCastle(byte castle, byte* pots);
void setTargetToCastle(byte castle, byte motor_index, byte value);
void sendRS485(byte to_id, byte cmd, byte len, byte* data);


void setup() {
  Serial.begin(9600); // RS485 baud rate
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

  pinMode(CASTLE_UP_PIN, INPUT_PULLUP);
  pinMode(CASTLE_DOWN_PIN, INPUT_PULLUP);

  Wire.begin(); // I2C master

  // Initialize targets to invalid
  memset(targets, 255, sizeof(targets));

  // Wait for boards to initialize
  delay(2000);
  switchCastle(current_castle); // Initial setup
  initial_setup_done = true;
}

void loop() {
  handleCastleSwitch();
  pollTargets();
  if (millis() - last_poll >= POLL_INTERVAL) {
    last_poll = millis();
    pollActualsAndUpdate();
  }
}

void handleCastleSwitch() {
  unsigned long current_time = millis();
  if (current_time - last_button_time > DEBOUNCE_TIME) {
    if (digitalRead(CASTLE_UP_PIN) == LOW) {
      current_castle = min(4, current_castle + 1);
      switchCastle(current_castle);
      last_button_time = current_time;
    } else if (digitalRead(CASTLE_DOWN_PIN) == LOW) {
      current_castle = max(1, current_castle - 1);
      switchCastle(current_castle);
      last_button_time = current_time;
    }
  }
}

void switchCastle(byte castle) {
  // Get pots for new castle
  byte pots[22];
  if (getPotsFromCastle(castle, pots)) {
    // Set targets and actuals to pots
    memcpy(targets, pots, sizeof(pots));
    memcpy(actuals, pots, sizeof(pots));
    // Send to displays as type 0 (target) and type 1 (actual)
    sendToDisplays(0, pots); // targets
    sendToDisplays(1, pots); // actuals
  }
}

void pollTargets() {
  for (byte d = 0; d < NUM_DISPLAY_BOARDS; d++) {
    byte addr = DISPLAY_BASE_ADDR + d;
    Wire.requestFrom(addr, (byte)4);
    if (Wire.available() == 4) {
      byte ch1 = Wire.read();
      byte v1 = Wire.read();
      byte ch2 = Wire.read();
      byte v2 = Wire.read();
      if (ch1 == 1 && ch2 == 2) {
        byte index1 = d * 2;
        byte index2 = d * 2 + 1;
        if (targets[index1] != v1) {
          targets[index1] = v1;
          setTargetToCastle(current_castle, index1, v1);
          Serial.println("Current castle: " + current_castle);
          Serial.println("Index1: " + index1);
          Serial.println("value1: " + v1);
        }
        if (targets[index2] != v2) {
          targets[index2] = v2;
          setTargetToCastle(current_castle, index2, v2);
          Serial.println("Current castle: " + current_castle);
          Serial.println("Index2: " + index2);
          Serial.println("value2: " + v2);
        }
      }
    }
  }
}

void pollActualsAndUpdate() {
  byte pots[22];
  if (getPotsFromCastle(current_castle, pots)) {
    memcpy(actuals, pots, sizeof(pots));
    sendToDisplays(1, pots); // Send actuals type 1
  }
}

void sendToDisplays(byte type, byte* values) {
  for (byte d = 0; d < NUM_DISPLAY_BOARDS; d++) {
    byte addr = DISPLAY_BASE_ADDR + d;
    byte index1 = d * 2;
    byte index2 = d * 2 + 1;

    Wire.beginTransmission(addr);
    Wire.write(type);
    Wire.write((byte)1);
    Wire.write(values[index1]);
    Wire.endTransmission();

    Wire.beginTransmission(addr);
    Wire.write(type);
    Wire.write((byte)2);
    Wire.write(values[index2]);
    Wire.endTransmission();
  }
}

bool getPotsFromCastle(byte castle, byte* pots) {
  byte cmd = 1;
  byte len = 0;
  byte dummy[0];
  sendRS485(castle, cmd, len, dummy);

  // Wait for response
  unsigned long start_time = millis();
  while (millis() - start_time < 500) {
    if (Serial.available() >= 28) { // 6 + 22 (start,id,cmd,len=22,data22,chk,end)
      byte incoming = Serial.read();
      if (incoming == START_BYTE) {
        byte id = Serial.read();
        byte rcmd = Serial.read();
        byte rlen = Serial.read();
        if (id == castle && rcmd == 1 && rlen == 22) {
          byte data[22];
          for (byte i = 0; i < 22; i++) {
            data[i] = Serial.read();
          }
          byte checksum = Serial.read();
          byte end = Serial.read();

          byte calc_checksum = id ^ rcmd ^ rlen;
          for (byte i = 0; i < 22; i++) {
            calc_checksum ^= data[i];
          }
          if (end == END_BYTE && checksum == calc_checksum) {
            memcpy(pots, data, 22);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void setTargetToCastle(byte castle, byte motor_index, byte value) {
  byte cmd = 2;
  byte len = 2;
  byte data[2] = {motor_index, value};
  sendRS485(castle, cmd, len, data);

  // Optional: wait for ack, but for simplicity, assume sent
}

void sendRS485(byte to_id, byte cmd, byte len, byte* data) {
  digitalWrite(DE_PIN, HIGH);
  Serial.write(START_BYTE);
  Serial.write(to_id);
  Serial.write(cmd);
  Serial.write(len);
  byte checksum = to_id ^ cmd ^ len;
  for (byte i = 0; i < len; i++) {
    Serial.write(data[i]);
    checksum ^= data[i];
  }
  Serial.write(checksum);
  Serial.write(END_BYTE);
  Serial.flush();
  digitalWrite(DE_PIN, LOW);
}