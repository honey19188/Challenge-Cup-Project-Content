#ifndef EFFECTS_H
#define EFFECTS_H

#include <Adafruit_NeoPixel.h>

class Effects {
private:
  Adafruit_NeoPixel *strip;
  int counter = 0;
  int hue = 0;
  
  void effect0() {
    for (int i = 0; i < strip->numPixels(); i++) {
      int brightness = (sin(i * 0.3 + counter * 0.1) + 1) * 127;
      strip->setPixelColor(i, strip->Color(brightness, brightness * 0.3, 0));
    }
    strip->show();
    counter++;
  }
  
  void effect1() {
    for (int i = 0; i < strip->numPixels(); i++) {
      int brightness = (sin(i * 0.2 + counter * 0.05) + 1) * 127;
      strip->setPixelColor(i, strip->Color(brightness, brightness, brightness));
    }
    strip->show();
    counter++;
  }
  
  void effect2() {
    int color1 = (hue % 360) * 256 / 360;
    int color2 = ((hue + 180) % 360) * 256 / 360;
    
    for (int i = 0; i < strip->numPixels(); i++) {
      if (i % 2 == 0) {
        strip->setPixelColor(i, strip->ColorHSV(color1, 255, 128));
      } else {
        strip->setPixelColor(i, strip->ColorHSV(color2, 255, 128));
      }
    }
    strip->show();
    hue += 5;
  }
  
  void effect3() {
    int startHue = hue % 360;
    for (int i = 0; i < strip->numPixels(); i++) {
      int pixelHue = (startHue + i * 5) % 360;
      strip->setPixelColor(i, strip->ColorHSV(pixelHue * 256 / 360, 255, 150));
    }
    strip->show();
    hue += 3;
  }
  
  void effect4() {
    strip->clear();
    int pos = counter % strip->numPixels();
    for (int i = 0; i < 5; i++) {
      int idx = (pos + i) % strip->numPixels();
      int brightness = 255 - i * 50;
      strip->setPixelColor(idx, strip->Color(brightness, brightness, brightness));
    }
    strip->show();
    counter++;
  }
  
  void effect5() {
    int pulse = (sin(counter * 0.1) + 1) * 127;
    for (int i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, strip->Color(pulse, 0, pulse));
    }
    strip->show();
    counter++;
  }
  
  void effect6() {
    if (counter % 10 == 0) {
      int pos = random(strip->numPixels());
      int color = random(360);
      for (int i = 0; i < 8; i++) {
        if (pos - i >= 0) strip->setPixelColor(pos - i, strip->ColorHSV(color * 256 / 360, 255, 200 - i * 25));
        if (pos + i < strip->numPixels()) strip->setPixelColor(pos + i, strip->ColorHSV(color * 256 / 360, 255, 200 - i * 25));
      }
    }
    strip->show();
    counter++;
    for (int i = 0; i < strip->numPixels(); i++) {
      uint32_t c = strip->getPixelColor(i);
      uint8_t r = (c >> 16) & 0xFF;
      uint8_t g = (c >> 8) & 0xFF;
      uint8_t b = c & 0xFF;
      r = max(0, r - 5);
      g = max(0, g - 5);
      b = max(0, b - 5);
      strip->setPixelColor(i, strip->Color(r, g, b));
    }
  }
  
  void effect7() {
    strip->clear();
    int tail = counter % (strip->numPixels() * 2);
    for (int i = 0; i < 10; i++) {
      int pos = tail - i;
      if (pos >= 0 && pos < strip->numPixels()) {
        int brightness = 255 - i * 25;
        strip->setPixelColor(pos, strip->ColorHSV((hue + i * 20) % 360 * 256 / 360, 255, brightness));
      }
    }
    strip->show();
    counter++;
    hue += 2;
  }

public:
  Effects(Adafruit_NeoPixel *s) : strip(s) {}
  
  void begin() {
    counter = 0;
    hue = 0;
  }
  
  void runEffect(int effect) {
    switch (effect) {
      case 0: effect0(); break;
      case 1: effect1(); break;
      case 2: effect2(); break;
      case 3: effect3(); break;
      case 4: effect4(); break;
      case 5: effect5(); break;
      case 6: effect6(); break;
      case 7: effect7(); break;
    }
  }
};

#endif
