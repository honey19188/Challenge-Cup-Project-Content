#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

class OLEDDisplay {
private:
  const char* effectNames[8] = {
    "盛世华诞",
    "普天同庆",
    "渐隐交替",
    "双色渐变",
    "扫描光带",
    "心跳脉冲",
    "烟花绽放",
    "拖尾彗尾"
  };
  
  const char* airQualityNames[4] = {
    "优",
    "良",
    "中",
    "差"
  };

public:
  void update(int mode, int effect, bool autoTour, bool lightOn, int temp, int air) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Mode: ");
    display.print(mode == 0 ? "Temp" : "Effect");
    
    display.setCursor(0, 12);
    if (mode == 0) {
      display.print("Temp: ");
      display.print(temp);
      display.print("C");
    } else {
      display.print("Scene: ");
      display.print(effectNames[effect]);
    }
    
    display.setCursor(0, 24);
    display.print("Auto: ");
    display.print(autoTour ? "ON" : "OFF");
    
    display.setCursor(0, 36);
    display.print("Light: ");
    display.print(lightOn ? "ON" : "OFF");
    
    display.setCursor(0, 48);
    display.print("Air: ");
    display.print(airQualityNames[constrain(air, 0, 3)]);
    
    display.display();
  }
};

#endif
