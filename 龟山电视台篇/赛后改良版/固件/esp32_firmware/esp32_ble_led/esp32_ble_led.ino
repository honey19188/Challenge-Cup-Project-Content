#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>

// ==================== 引脚定义 (ESP32-WROOM-32E) ====================
#define LED_PIN     25
#define VOICE_RX    17
#define VOICE_TX    16

// ==================== 灯带配置 ====================
#define NUM_LEDS    64
int brightness = 140;

// ==================== BLE UART 配置 ====================
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define DEVICE_NAME  "ESP32LED"

// ==================== 模式枚举 ====================
#define MODE_OFF      0
#define MODE_REAL     1
#define MODE_FESTIVAL 2
#define MODE_RAINBOW  3
#define MODE_BREATH   4
#define MODE_TOP_FLASH 5

// ==================== 硬件对象 ====================
Adafruit_NeoPixel np(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial VoiceSerial(1);
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
bool deviceConnected = false;
String rxValue = "";         // BLE 收到的原始字符串

// BLE 服务器回调
class MyServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("客户端已连接");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("客户端断开，重新广播...");
    // 先停止当前的广播（防止重复）
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->stop();
    // 延迟 1 秒后重新开始广播，确保不频繁重连
    delay(1000);
    pAdv->start();
    Serial.println("BLE 重新开始广播，可被搜索");
  }
};

// BLE RX 特征值回调 —— 仅保存原始字符串，loop 中统一处理
class MyRXCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
    rxValue = pChar->getValue().c_str();
  }
};

// ==================== 灯光状态 ====================
int mode = MODE_REAL;
uint32_t static_color = 0;
int step = 0; unsigned long step_timer = 0; uint8_t hue = 0;
int b_val = 30; int b_dir = 3; int p = 0; bool f = false;

// ==================== 辅助函数 ====================
void setAll(uint32_t c) {
  for (int i = 0; i < NUM_LEDS; i++) np.setPixelColor(i, c);
}

void resetAnim() {
  step = 0; step_timer = millis(); hue = 0;
  b_val = 30; b_dir = 3; p = 0; f = false;
}

// 2字符十六进制字符串 -> 字节
uint8_t hexToByte(const String& s) {
  if (s.length() < 2) return 0;
  uint8_t r = 0;
  for (int i = 0; i < 2; i++) {
    char c = s[i];
    r <<= 4;
    if (c >= '0' && c <= '9')      r |= (c - '0');
    else if (c >= 'A' && c <= 'F') r |= (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') r |= (c - 'a' + 10);
    else return 0;
  }
  return r;
}

// ==================== 指令处理（loop 中调用） ====================
void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.print("指令: ");
  Serial.println(cmd);

  static_color = 0;

  // ---- 方式1: 文本指令 (旧兼容) ----
  String lower = cmd;
  lower.toLowerCase();
  if      (lower == "off")      { mode = MODE_OFF; }
  else if (lower == "real")     { mode = MODE_REAL; }
  else if (lower == "festival") { mode = MODE_FESTIVAL; }
  else if (lower == "rainbow")  { mode = MODE_RAINBOW; }
  else if (lower == "breath")   { mode = MODE_BREATH; }
  else if (lower == "top")      { mode = MODE_TOP_FLASH; }
  else if (lower == "white")    { static_color = np.Color(255,255,255); mode = MODE_OFF; }
  else if (lower == "red")      { static_color = np.Color(255,0,0);     mode = MODE_OFF; }
  else if (lower == "blue")     { static_color = np.Color(0,0,255);     mode = MODE_OFF; }
  else if (lower == "warm")     { static_color = np.Color(255,220,160); mode = MODE_OFF; }
  else {
    // ---- 方式2: 十六进制字符串指令 (Android App) ----
    if (cmd.length() >= 2) {
      uint8_t h = hexToByte(cmd);
      switch (h) {
        case 0x00: mode = MODE_OFF; break;
        case 0x01: mode = MODE_REAL; break;
        case 0x02: mode = MODE_FESTIVAL; break;
        case 0x03: mode = MODE_RAINBOW; break;
        case 0x04: mode = MODE_BREATH; break;
        case 0x05: mode = MODE_TOP_FLASH; break;
        case 0x06: mode = MODE_REAL; break;
        case 0x07: mode = MODE_FESTIVAL; break;
        case 0x08: mode = MODE_RAINBOW; break;
        case 0x09: mode = MODE_BREATH; break;
        case 0x10: mode = MODE_TOP_FLASH; break;
        case 0xFE: mode = MODE_RAINBOW; break;

        // 扩展功能 —— 通过串口转发给语音模块
        case 0xF0: VoiceSerial.write(0xF0); Serial.println("窗帘打开"); return;
        case 0xF1: VoiceSerial.write(0xF1); Serial.println("窗帘关闭"); return;

        // 查询 —— 通过 TX 通知回传
        case 0x50:
          if (pTxCharacteristic && deviceConnected) {
            pTxCharacteristic->setValue("温度:24C 湿度:64% 空气:66");
            pTxCharacteristic->notify();
          }
          return;
        case 0x51:
          if (pTxCharacteristic && deviceConnected) {
            pTxCharacteristic->setValue("2026-05-09");
            pTxCharacteristic->notify();
          }
          return;
        default: return;  // 无法识别的指令
      }
    } else {
      return;
    }
  }

  resetAnim();
}

// ==================== 语音识别串口 ====================
void checkVoice() {
  if (!VoiceSerial.available()) return;
  uint8_t b = VoiceSerial.read();
  static_color = 0;
  bool reset = true;
  switch (b) {
    case 0x00: mode = MODE_OFF; break;
    case 0x01: mode = MODE_REAL; break;
    case 0x02: mode = MODE_FESTIVAL; break;
    case 0x03: mode = MODE_RAINBOW; break;
    case 0x04: mode = MODE_BREATH; break;
    case 0x05: mode = MODE_TOP_FLASH; break;
    case 0x07: static_color = np.Color(255,255,255); mode = MODE_OFF; break;
    case 0x08: static_color = np.Color(255,0,0);     mode = MODE_OFF; break;
    case 0x09: static_color = np.Color(0,0,255);     mode = MODE_OFF; break;
    case 0x0A: static_color = np.Color(255,220,160); mode = MODE_OFF; break;
    default: reset = false; break;
  }
  if (reset) resetAnim();
}

// ==================== 灯光秀效果 ====================
void realShow() {
  if (millis() - step_timer > 3000) { step_timer = millis(); step = (step + 1) % 3; p = 0; f = false; }
  setAll(0);
  if (step == 0) {
    for (int i = 0; i < 16; i++) np.setPixelColor(i, np.Color(255,210,160));
  } else if (step == 1) {
    for (int i = 0; i < 24; i++) {
      int idx = 16 + i;
      if ((i - p + 24) % 24 < 4) {
        float bs = 1.0 - ((i - p + 24) % 24) / 4.0;
        np.setPixelColor(idx, np.Color((int)(30*bs*brightness/255), (int)(150*bs*brightness/255), (int)(255*bs*brightness/255)));
      }
    }
    p = (p + 1) % 24; delay(80);
  } else {
    uint32_t c = f ? np.Color(255,0,0) : np.Color(100,0,0);
    for (int i = 40; i < 60; i++) np.setPixelColor(i, c);
    f = !f; delay(200);
  }
  np.show();
}

void festivalShow() {
  for (int i = 0; i < 16; i++) np.setPixelColor(i, np.Color(255,200,0));
  for (int i = 0; i < 24; i++)
    np.setPixelColor(16+i, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV((hue+i*10)%256*256, 255, 220)));
  uint32_t top = ((millis()/300)%2) ? np.Color(255,0,0) : np.Color(50,0,0);
  for (int i = 40; i < 60; i++) np.setPixelColor(i, top);
  hue = (hue+1)%256; np.show(); delay(30);
}

void rainbowShow() {
  for (int i = 0; i < NUM_LEDS; i++)
    np.setPixelColor(i, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV((hue+i*5)%256*256, 255, 255)));
  hue = (hue+1)%256; np.show(); delay(25);
}

void breathShow() {
  float scale = b_val / 255.0;
  setAll(np.Color((int)(60*scale), (int)(120*scale), (int)(220*scale)));
  np.show();
  b_val += b_dir;
  if (b_val <= 30 || b_val >= brightness) { b_dir *= -1; b_val = constrain(b_val, 30, brightness); }
  delay(25);
}

void topFlashShow() {
  for (int i = 0; i < 16; i++) np.setPixelColor(i, np.Color(255,255,255));
  for (int i = 16; i < 40; i++) np.setPixelColor(i, 0);
  uint32_t top = ((millis()/150)%2) ? np.Color(255,0,0) : 0;
  for (int i = 40; i < 60; i++) np.setPixelColor(i, top);
  np.show();
}

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  VoiceSerial.begin(115200, SERIAL_8N1, VOICE_RX, VOICE_TX);

  np.begin();
  np.setBrightness(brightness);
  np.clear();
  np.show();

  // BLE 初始化
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCB());

  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  pTxCharacteristic = pService->createCharacteristic(
                        BLEUUID(CHARACTERISTIC_UUID_TX),
                        BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRX = pService->createCharacteristic(
                        BLEUUID(CHARACTERISTIC_UUID_RX),
                        BLECharacteristic::PROPERTY_WRITE);
  pRX->setCallbacks(new MyRXCB());
  pService->start();

  // 广播 —— 包含 Service UUID，方便 Android App 识别
  BLEAdvertising *pAdv = pServer->getAdvertising();
  BLEAdvertisementData adv;
  adv.setName(DEVICE_NAME);
  adv.setAppearance(0x0080);
  adv.setCompleteServices(BLEUUID(SERVICE_UUID));
  pAdv->setAdvertisementData(adv);
  pAdv->start();

  Serial.println("BLE 已启动: " + String(DEVICE_NAME));
  Serial.println("Service UUID: " + String(SERVICE_UUID));
}

// ==================== Loop ====================
void loop() {
  // 1) 处理 BLE 接收到的指令
  if (rxValue.length() > 0) {
    String cmd = rxValue;
    rxValue = "";
    processCommand(cmd);
  }

  // 2) 语音
  checkVoice();

  // 3) 灯光渲染
  if (static_color) {
    setAll(static_color);
    np.show();
    delay(50);
    return;
  }

  switch (mode) {
    case MODE_OFF:       setAll(0); np.show(); break;
    case MODE_REAL:      realShow(); break;
    case MODE_FESTIVAL:  festivalShow(); break;
    case MODE_RAINBOW:   rainbowShow(); break;
    case MODE_BREATH:    breathShow(); break;
    case MODE_TOP_FLASH: topFlashShow(); break;
  }

  delay(1);
}
