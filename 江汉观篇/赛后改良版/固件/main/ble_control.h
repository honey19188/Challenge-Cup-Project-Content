#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "0000FFE0-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "0000FFE1-0000-1000-8000-00805F9B34FB"

typedef void (*CommandCallback)(String);

class BLEControl {
private:
  BLEServer *pServer = nullptr;
  BLECharacteristic *pCharacteristic = nullptr;
  CommandCallback callback = nullptr;
  bool deviceConnected = false;
  
  class MyServerCallbacks : public BLEServerCallbacks {
    BLEControl *parent;
  public:
    MyServerCallbacks(BLEControl *p) : parent(p) {}
    
    void onConnect(BLEServer* pServer) {
      parent->deviceConnected = true;
    }
    
    void onDisconnect(BLEServer* pServer) {
      parent->deviceConnected = false;
      pServer->startAdvertising();
    }
  };
  
  class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    BLEControl *parent;
  public:
    MyCharacteristicCallbacks(BLEControl *p) : parent(p) {}
    
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      if (value.length() > 0 && parent->callback) {
        parent->callback(value);
      }
    }
  };

public:
  void begin(CommandCallback cb) {
    callback = cb;
    
    BLEDevice::init("ESP32_BLE");
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(this));
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_INDICATE
    );
    
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(this));
    pCharacteristic->addDescriptor(new BLE2902());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
  }
  
  void sendStatus(int mode, int effect, bool autoTour, bool lightOn, int temp, int air) {
    if (!deviceConnected || !pCharacteristic) return;
    
    String status = "mode:" + String(mode) + 
                    ",scene:" + String(effect) + 
                    ",auto:" + String(autoTour ? 1 : 0) + 
                    ",light:" + String(lightOn ? 1 : 0) + 
                    ",temp:" + String(temp) + 
                    ",air:" + String(air);
    
    pCharacteristic->setValue(status.c_str());
    pCharacteristic->notify();
  }
  
  bool isConnected() {
    return deviceConnected;
  }
};

#endif
