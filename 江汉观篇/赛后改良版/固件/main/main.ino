#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>

#include "effects.h"
#include "su03t.h"
#include "ble_control.h"
#include "oled_display.h"

#define LED_PIN 18
#define LED_COUNT 60
#define MQ135_PIN 4
#define BOOT_PIN 0

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

SU03T voiceModule;
BLEControl ble;
OLEDDisplay oled;

Effects effects(&strip);

enum Mode {
  MODE_TEMP,
  MODE_EFFECT
};

enum TempPreset {
  TEMP_HOT = 35,
  TEMP_WARM = 22,
  TEMP_COOL = 10,
  TEMP_COLD = -15
};

Mode currentMode = MODE_TEMP;
int currentTemp = TEMP_WARM;
int currentEffect = 0;
bool autoTour = false;
bool lightOn = true;
int airQuality = 1;

unsigned long lastEffectTime = 0;
unsigned long lastAirTime = 0;
unsigned long lastKeyPress = 0;
int keyPressCount = 0;

uint32_t tempColor(int temp) {
  if (temp >= 35) return strip.Color(255, 0, 0);
  if (temp >= 22) return strip.Color(255, 200, 0);
  if (temp >= 10) return strip.Color(0, 255, 0);
  return strip.Color(0, 100, 255);
}

void updateLights() {
  if (!lightOn) {
    strip.clear();
    strip.show();
    return;
  }
  
  if (currentMode == MODE_TEMP) {
    uint32_t color = tempColor(currentTemp);
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
  } else {
    effects.runEffect(currentEffect);
  }
}

void readMQ135() {
  int rawValue = analogRead(MQ135_PIN);
  if (rawValue < 200) airQuality = 0;
  else if (rawValue < 400) airQuality = 1;
  else if (rawValue < 600) airQuality = 2;
  else airQuality = 3;
}

void handleVoiceCommand(uint8_t cmd) {
  switch (cmd) {
    case 0x01: lightOn = false; break;
    case 0x02: lightOn = true; break;
    case 0x03: currentMode = MODE_EFFECT; currentEffect = 0; break;
    case 0x04: currentMode = MODE_EFFECT; currentEffect = 1; break;
    case 0x05: currentMode = MODE_EFFECT; currentEffect = 2; break;
    case 0x06: currentMode = MODE_EFFECT; currentEffect = 3; break;
    case 0x07: currentMode = MODE_EFFECT; currentEffect = 4; break;
    case 0x08: currentMode = MODE_EFFECT; currentEffect = 5; break;
    case 0x09: currentMode = MODE_EFFECT; currentEffect = 6; break;
    case 0x0A: currentMode = MODE_EFFECT; currentEffect = 7; break;
    case 0x0B: autoTour = !autoTour; break;
    case 0x0C: currentMode = MODE_TEMP; currentTemp = TEMP_WARM; break;
    case 0x0D: voiceModule.speak("当前温度设定为22摄氏度"); break;
    case 0x0E: voiceModule.speakTime(); break;
  }
  ble.sendStatus(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
}

void handleBLECommand(String cmd) {
  if (cmd == "light_on") lightOn = true;
  else if (cmd == "light_off") lightOn = false;
  else if (cmd.startsWith("temp:")) {
    currentMode = MODE_TEMP;
    currentTemp = cmd.substring(5).toInt();
  } else if (cmd.startsWith("scene:")) {
    currentMode = MODE_EFFECT;
    currentEffect = constrain(cmd.substring(6).toInt(), 0, 7);
  } else if (cmd == "auto") autoTour = !autoTour;
  else if (cmd == "report") {
    String status = String(currentMode) + "," + String(currentEffect) + "," + 
                    String(autoTour) + "," + String(lightOn) + "," + 
                    String(currentTemp) + "," + String(airQuality);
    voiceModule.speak("当前模式" + String(currentMode == MODE_TEMP ? "温度" : "效果") + 
                      "温度" + String(currentTemp) + "度");
  } else if (cmd == "chime") voiceModule.speakTime();
  
  ble.sendStatus(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
}

void checkBootKey() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BOOT_PIN);
  
  if (lastState == HIGH && currentState == LOW) {
    unsigned long now = millis();
    if (now - lastKeyPress < 500) {
      keyPressCount++;
      if (keyPressCount >= 2) {
        voiceModule.speak("当前模式" + String(currentMode == MODE_TEMP ? "温度" : "效果") + 
                          "温度" + String(currentTemp) + "度");
        keyPressCount = 0;
      }
    } else {
      keyPressCount = 1;
    }
    lastKeyPress = now;
    
    if (keyPressCount == 1) {
      currentMode = (Mode)(!currentMode);
      if (currentMode == MODE_TEMP) {
        voiceModule.speak("已切换到温度模式");
      } else {
        voiceModule.speak("已切换到效果模式");
      }
      ble.sendStatus(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
    }
  }
  lastState = currentState;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(42, 41);
  Wire.setClock(100000);
  
  pinMode(BOOT_PIN, INPUT_PULLUP);
  
  strip.begin();
  strip.setBrightness(128);
  strip.show();
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }
  display.clearDisplay();
  
  voiceModule.begin();
  ble.begin(handleBLECommand);
  effects.begin();
  
  voiceModule.speak("系统已启动");
}

void loop() {
  unsigned long now = millis();
  
  checkBootKey();
  
  if (now - lastAirTime >= 1800000) {
    readMQ135();
    lastAirTime = now;
    ble.sendStatus(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
  }
  
  if (autoTour && currentMode == MODE_EFFECT && now - lastEffectTime >= 5000) {
    currentEffect = (currentEffect + 1) % 8;
    lastEffectTime = now;
    ble.sendStatus(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
  }
  
  uint8_t voiceCmd = voiceModule.readCommand();
  if (voiceCmd != 0) {
    handleVoiceCommand(voiceCmd);
  }
  
  updateLights();
  
  oled.update(currentMode, currentEffect, autoTour, lightOn, currentTemp, airQuality);
  
  delay(80);
}
