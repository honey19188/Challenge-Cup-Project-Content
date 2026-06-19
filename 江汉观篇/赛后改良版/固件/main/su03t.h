#ifndef SU03T_H
#define SU03T_H

#include <HardwareSerial.h>

class SU03T {
private:
  HardwareSerial *serial;
  uint8_t buffer[10];
  int bufferIndex = 0;
  
public:
  void begin() {
    serial = new HardwareSerial(1);
    serial->begin(115200, SERIAL_8N1, 17, 18);
    bufferIndex = 0;
  }
  
  uint8_t readCommand() {
    while (serial->available() > 0) {
      uint8_t byte = serial->read();
      
      if (bufferIndex == 0 && byte != 0xAA) continue;
      
      buffer[bufferIndex++] = byte;
      
      if (bufferIndex == 6) {
        if (buffer[0] == 0xAA && buffer[1] == 0x55 && 
            buffer[4] == 0x00 && buffer[5] == 0x55 && buffer[6] == 0xAA) {
          uint8_t cmd = buffer[2];
          bufferIndex = 0;
          return cmd;
        }
        bufferIndex = 0;
      }
    }
    return 0;
  }
  
  void speak(String text) {
    serial->print("{\"play\":\"");
    serial->print(text);
    serial->println("\"}");
  }
  
  void speakTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      speak("无法获取时间");
      return;
    }
    
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    
    String hourStr = String(hour);
    String minuteStr = String(minute);
    
    speak("现在时间" + hourStr + "点" + minuteStr + "分");
  }
};

#endif
