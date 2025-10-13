// CastleBoard Code
#include <Arduino.h>
#include <Wire.h>

const byte CASTLE_ID = 1; // Change this for each CastleBoard: 1 to 4
const byte START_BYTE = 0xAA;
const byte END_BYTE = 0xBB;
const byte MOTOR_BASE_ADDR = 0x08; // MotorBoards from 0x08 to 0x12 (11 boards)
const byte NUM_MOTOR_BOARDS = 11;

void processCommand(byte cmd, byte len, byte* data);
void fetchAllPots();
void setMotorTarget(byte motor_index, byte value);
void sendResponse(byte id, byte cmd, byte len, byte* data);

byte pot_values[22]; // 11 boards * 2 channels

void setup() {
  Serial.begin(9600); // For terminal output
  Serial1.begin(9600);  // Hardware serial for communication

  Wire.begin(); // I2C master for MotorBoards
  Serial.print("CastleBoard ");
  Serial.print(CASTLE_ID);
  Serial.println(" started");
}

void loop() {
  if (Serial1.available() > 0) {
    byte incoming = Serial1.read();
    if (incoming == START_BYTE) {
      while (Serial1.available() < 3); // Wait for ID, CMD, LEN
      byte id = Serial1.read();
      if (id == CASTLE_ID) {
        byte cmd = Serial1.read();
        byte len = Serial1.read();
        byte data[22];
        while (Serial1.available() < len + 2); // Wait for DATA + CHECKSUM + END
        for (byte i = 0; i < len; i++) {
          data[i] = Serial1.read();
        }
        byte checksum = Serial1.read();
        byte end = Serial1.read();

        byte calc_checksum = id ^ cmd ^ len;
        for (byte i = 0; i < len; i++) {
          calc_checksum ^= data[i];
        }

        if (end == END_BYTE && checksum == calc_checksum) {
          processCommand(cmd, len, data);
        } else {
          Serial.println("Castle: Invalid packet");
        }
      }
    }
  }
  
}

void processCommand(byte cmd, byte len, byte* data) {
  if (cmd == 1) { // Get all pots
    Serial.println("Recebeu CMD1 lê os potenciometros");
    fetchAllPots();
    delay(500);
    sendResponse(CASTLE_ID, 1, 22, pot_values);
  } else if (cmd == 2 && len == 2) { // Set target
    Serial.println("Recebeu CMD2 Irá mover motor");
    byte motor_index = data[0]; // 0-21
    byte value = data[1]; // 0-99
    setMotorTarget(motor_index, value);
    sendResponse(CASTLE_ID, 2, 0, NULL);
  }
  if (!Wire.available()){
    Serial.println("Desconectou, reconectando");
    Serial1.end();
    delay(500);
    Serial1.begin(9600);
    delay(500);
    fetchAllPots();
    sendResponse(CASTLE_ID, 1, 22, pot_values);
  }
}

void fetchAllPots() {
  for (byte board = 0; board < NUM_MOTOR_BOARDS; board++) {
    unsigned long start_time = millis();
    byte addr = MOTOR_BASE_ADDR + board;
    Serial.print("Requesting data from MotorBoard 0x");
    Serial.println(addr, HEX);

    Wire.requestFrom(addr, (byte)4);
    int bytesAvailable = Wire.available();
    Serial.print("Bytes received: ");
    Serial.println(bytesAvailable);

    if (bytesAvailable == 4) {
      byte ch1 = Wire.read();
      byte p1 = Wire.read();
      byte ch2 = Wire.read();
      byte p2 = Wire.read();

      Serial.print("MotorBoard 0x");
      Serial.print(addr, HEX);
      Serial.print(" - Ch1 ID: ");
      Serial.print(ch1);
      Serial.print(", Pot1: ");
      Serial.print(p1);
      Serial.print(", Ch2 ID: ");
      Serial.print(ch2);
      Serial.print(", Pot2: ");
      Serial.println(p2);

      if (ch1 == 1 && ch2 == 2) {
        // Scale 0-255 to 0-99 for display
        pot_values[board * 2] = map(p1, 0, 255, 0, 99);
        pot_values[board * 2 + 1] = map(p2, 0, 255, 0, 99);
        Serial.print("Scaled values - Pot1: ");
        Serial.print(pot_values[board * 2]);
        Serial.print(", Pot2: ");
        Serial.println(pot_values[board * 2 + 1]);
      } else {
        Serial.println("Invalid channel IDs received");
      }
    } else {
      Serial.print("Castle: No response or incorrect data from MotorBoard 0x");
      Serial.println(addr, HEX);
      // Clear any received bytes
      while (Wire.available()) {
        Serial.print("Discarded byte: 0x");
        Serial.println(Wire.read(), HEX);
      }
    }
  }
}

void setMotorTarget(byte motor_index, byte value) {
  byte board = motor_index / 2;
  byte channel = (motor_index % 2) + 1;
  byte addr = MOTOR_BASE_ADDR + board;
  // Scale 0-99 to 0-255 for MotorBoard
  byte scaled_value = map(value, 0, 99, 0, 255);
  Wire.beginTransmission(addr);
  Wire.write(channel);
  Wire.write(scaled_value);
  Wire.endTransmission();
  Serial.print("Castle: Set Motor ");
  Serial.print(motor_index);
  Serial.print(" to ");
  Serial.println(value);
}

void sendResponse(byte id, byte cmd, byte len, byte* data) {
  Serial1.write(START_BYTE);
  Serial1.write(id);
  Serial1.write(cmd);
  Serial1.write(len);
  byte checksum = id ^ cmd ^ len;
  for (byte i = 0; i < len; i++) {
    Serial1.write(data[i]);
    checksum ^= data[i];
  }
  Serial1.write(checksum);
  Serial1.write(END_BYTE);
  Serial1.flush();
}
