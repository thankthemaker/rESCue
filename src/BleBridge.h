#ifndef __BLUETOOTH_BRIDGE_H__
#define __BLUETOOTH_BRIDGE_H__

#ifdef ESP32

#include "config.h"
#include "BluetoothSerial.h"
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e" 
//#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
//#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

extern Stream* vescSerial;
extern bool deviceConnected; 

class BleBridge {
    public:
        BleBridge();
        void init(Stream *vesc);
        void loop();
};

class BleServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
#if DEBUG > 0
      Serial.println("BLE device connected");
#endif
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
#if DEBUG > 0
     Serial.println("BLE device disconnected");
#endif
     deviceConnected = false;
    }
};

class BleCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          vescSerial->write(rxValue[i]);
        }
      }
    }
};

#endif //ESP32
#endif //__BLUETOOTH_BRIDGE_H__