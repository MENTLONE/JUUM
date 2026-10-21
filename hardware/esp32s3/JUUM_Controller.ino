/*
  JUUM ESP32-S3 Hardware Controller — Rev 01
  USB CDC serial protocol: one JSON object per line, 115200 baud.
  Board: ESP32-S3 development board with native USB CDC enabled.
  Libraries: Adafruit MCP23X17, FastLED, ArduinoJson.
*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <FastLED.h>
#include <ArduinoJson.h>

static const uint32_t SERIAL_BAUD = 115200;
static const uint8_t POT_COUNT = 8;
static const uint8_t potPins[POT_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8};
static const uint8_t ENC_COUNT = 4;
static const uint8_t encA[ENC_COUNT] = {9, 11, 13, 15};
static const uint8_t encB[ENC_COUNT] = {10, 12, 14, 16};
static const uint8_t LED_PIN = 40;
static const uint8_t LED_COUNT = 16;
static const uint8_t I2C_SDA = 17;
static const uint8_t I2C_SCL = 18;
static const uint8_t MCP_STEPS_ADDR = 0x20;
static const uint8_t MCP_FUNCTIONS_ADDR = 0x21;

Adafruit_MCP23X17 stepsIO;
Adafruit_MCP23X17 functionIO;
CRGB leds[LED_COUNT];

uint16_t lastPot[POT_COUNT];
uint8_t lastEncoderState[ENC_COUNT];
int8_t encoderAccum[ENC_COUNT] = {0, 0, 0, 0};
bool lastStepPressed[16];
bool lastFunctionPressed[8];
uint32_t lastStepChange[16] = {0};
uint32_t lastFunctionChange[8] = {0};
bool stepEnabled[16] = {false};
int8_t playhead = -1;
String rxLine;

const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void sendEvent(const char* type, int id, int value = -1, const char* extraKey = nullptr, const char* extraValue = nullptr) {
  StaticJsonDocument<192> doc;
  doc["type"] = type;
  doc["id"] = id;
  if (value >= 0) doc["value"] = value;
  if (strcmp(type, "encoder") == 0) doc["delta"] = value;
  if (extraKey && extraValue) doc[extraKey] = extraValue;
  serializeJson(doc, Serial);
  Serial.println();
}

void sendHello() {
  StaticJsonDocument<128> doc;
  doc["type"] = "hello";
  doc["name"] = "JUUM ESP32-S3 Rev 01";
  doc["protocol"] = 1;
  serializeJson(doc, Serial);
  Serial.println();
}

void renderLeds() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    if (i == playhead) leds[i] = CRGB(255, 255, 255);
    else if (stepEnabled[i]) leds[i] = CRGB(0, 180, 255);
    else leds[i] = CRGB(0, 10, 20);
  }
  FastLED.show();
}

void processSerialLine(String line) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, line)) return;
  const char* type = doc["type"] | "";
  if (strcmp(type, "leds") == 0) {
    JsonArray steps = doc["steps"].as<JsonArray>();
    if (!steps.isNull()) {
      for (uint8_t i = 0; i < LED_COUNT && i < steps.size(); i++) stepEnabled[i] = steps[i].as<bool>();
    }
    playhead = doc["playhead"] | -1;
    renderLeds();
  }
}

void readSerialCommands() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\n') {
      processSerialLine(rxLine);
      rxLine = "";
    } else if (c != '\r' && rxLine.length() < 500) {
      rxLine += c;
    } else if (rxLine.length() >= 500) {
      rxLine = "";
    }
  }
}

void scanPots() {
  static uint32_t lastScan = 0;
  if (millis() - lastScan < 12) return;
  lastScan = millis();
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    const uint16_t value = analogRead(potPins[i]);
    if (lastPot[i] == 0xFFFF || abs((int)value - (int)lastPot[i]) >= 18) {
      lastPot[i] = value;
      StaticJsonDocument<96> doc;
      doc["type"] = "pot";
      doc["id"] = i;
      doc["value"] = value;
      serializeJson(doc, Serial);
      Serial.println();
    }
  }
}

void scanEncoders() {
  for (uint8_t i = 0; i < ENC_COUNT; i++) {
    const uint8_t current = (digitalRead(encA[i]) << 1) | digitalRead(encB[i]);
    const uint8_t transition = (lastEncoderState[i] << 2) | current;
    encoderAccum[i] += encoderTable[transition];
    lastEncoderState[i] = current;
    if (encoderAccum[i] >= 4) {
      sendEvent("encoder", i, 1);
      encoderAccum[i] = 0;
    } else if (encoderAccum[i] <= -4) {
      sendEvent("encoder", i, -1);
      encoderAccum[i] = 0;
    }
  }
}

void scanButtonExpander(Adafruit_MCP23X17& io, uint8_t count, bool* lastPressed, uint32_t* lastChange, bool isSteps) {
  for (uint8_t i = 0; i < count; i++) {
    const bool pressed = io.digitalRead(i) == LOW;
    if (pressed != lastPressed[i] && millis() - lastChange[i] > 24) {
      lastChange[i] = millis();
      lastPressed[i] = pressed;
      if (!pressed) continue; // send only on press
      if (isSteps) {
        sendEvent("step", i);
      } else if (i < 4) {
        sendEvent("encoderPress", i);
      } else {
        sendEvent("button", i - 4);
      }
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(700);
  analogReadResolution(12);
  for (uint8_t i = 0; i < POT_COUNT; i++) {
    pinMode(potPins[i], INPUT);
    lastPot[i] = 0xFFFF;
  }
  for (uint8_t i = 0; i < ENC_COUNT; i++) {
    pinMode(encA[i], INPUT_PULLUP);
    pinMode(encB[i], INPUT_PULLUP);
    lastEncoderState[i] = (digitalRead(encA[i]) << 1) | digitalRead(encB[i]);
  }
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!stepsIO.begin_I2C(MCP_STEPS_ADDR)) {
    Serial.println("{\"type\":\"error\",\"message\":\"MCP23017 step expander not found at 0x20\"}");
    while (true) delay(1000);
  }
  if (!functionIO.begin_I2C(MCP_FUNCTIONS_ADDR)) {
    Serial.println("{\"type\":\"error\",\"message\":\"MCP23017 function expander not found at 0x21\"}");
    while (true) delay(1000);
  }
  for (uint8_t i = 0; i < 16; i++) {
    stepsIO.pinMode(i, INPUT_PULLUP);
    lastStepPressed[i] = false;
  }
  for (uint8_t i = 0; i < 8; i++) {
    functionIO.pinMode(i, INPUT_PULLUP);
    lastFunctionPressed[i] = false;
  }
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(45);
  renderLeds();
  sendHello();
}

void loop() {
  readSerialCommands();
  scanPots();
  scanEncoders();
  scanButtonExpander(stepsIO, 16, lastStepPressed, lastStepChange, true);
  scanButtonExpander(functionIO, 8, lastFunctionPressed, lastFunctionChange, false);
  delay(1);
}
