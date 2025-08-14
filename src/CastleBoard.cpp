// CastleBoard Code
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>

const byte CASTLE_ID = 1; // Change this for each CastleBoard: 1 to 4
const byte DE_PIN = 12;   // RS485 Driver Enable
const byte DI_PIN = 11;   // RS485 Data In (TX)
const byte RO_PIN = 10;   // RS485 Receive Out (RX)
const byte START_BYTE = 0xAA;
const byte END_BYTE = 0xBB;
const byte MOTOR_BASE_ADDR = 0x08; // MotorBoards from 0x08 to 0x12 (11 boards)
const byte NUM_MOTOR_BOARDS = 11;

void processCommand(byte cmd, byte len, byte* data);
void fetchAllPots();
void setMotorTarget(byte motor_index, byte value);
void sendResponse(byte id, byte cmd, byte len, byte* data);

SoftwareSerial rs485(RO_PIN, DI_PIN); // RX, TX for RS485
byte pot_values[22]; // 11 boards * 2 channels

void setup() {
  Serial.begin(9600); // For terminal output
  rs485.begin(9600);  // RS485 communication
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW); // Receive mode

  Wire.begin(); // I2C master for MotorBoards
  Serial.print("CastleBoard ");
  Serial.print(CASTLE_ID);
  Serial.println(" started");
}

void loop() {
  if (rs485.available() > 0) {
    byte incoming = rs485.read();
    if (incoming == START_BYTE) {
      while (rs485.available() < 3); // Wait for ID, CMD, LEN
      byte id = rs485.read();
      if (id == CASTLE_ID) {
        byte cmd = rs485.read();
        byte len = rs485.read();
        byte data[22];
        while (rs485.available() < len + 2); // Wait for DATA + CHECKSUM + END
        for (byte i = 0; i < len; i++) {
          data[i] = rs485.read();
        }
        byte checksum = rs485.read();
        byte end = rs485.read();

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
    fetchAllPots();
    sendResponse(CASTLE_ID, 1, 22, pot_values);
  } else if (cmd == 2 && len == 2) { // Set target
    byte motor_index = data[0]; // 0-21
    byte value = data[1]; // 0-99
    setMotorTarget(motor_index, value);
    sendResponse(CASTLE_ID, 2, 0, NULL);
  }
}

void fetchAllPots() {
  for (byte board = 0; board < NUM_MOTOR_BOARDS; board++) {
    byte addr = MOTOR_BASE_ADDR + board;
    Wire.requestFrom(addr, (byte)4);
    if (Wire.available() == 4) {
      byte ch1 = Wire.read();
      byte p1 = Wire.read();
      byte ch2 = Wire.read();
      byte p2 = Wire.read();
      if (ch1 == 1 && ch2 == 2) {
        // Scale 0-255 to 0-99 for display
        pot_values[board * 2] = map(p1, 0, 255, 0, 99);
        pot_values[board * 2 + 1] = map(p2, 0, 255, 0, 99);
      }
      Serial.print("Address: " + addr);
      Serial.print("Ch1: " + ch1);
      Serial.print("Pot1: " + p1);
      Serial.print("Ch2: " + ch2);
      Serial.print("Pot2: " + p2);
    } else {
      Serial.print("Castle: No response from MotorBoard 0x");
      Serial.println(addr, HEX);
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
  digitalWrite(DE_PIN, HIGH);
  rs485.write(START_BYTE);
  rs485.write(id);
  rs485.write(cmd);
  rs485.write(len);
  byte checksum = id ^ cmd ^ len;
  for (byte i = 0; i < len; i++) {
    rs485.write(data[i]);
    checksum ^= data[i];
  }
  rs485.write(checksum);
  rs485.write(END_BYTE);
  rs485.flush();
  digitalWrite(DE_PIN, LOW);
}