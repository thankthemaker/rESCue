#ifndef BLUETOOTH_BRIDGE_H
#define BLUETOOTH_BRIDGE_H

#include <LoopbackStream.h>
#include "config.h"
#include "BluetoothSerial.h"
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UART service UUID, generated randomly
#define SERVICE_UUID           "0A7EFBDE-7E87-4CDC-AF88-49888AD819B0" 
#define CHARACTERISTIC_UUID_RX "0A7EFBDE-7E87-4CDC-AF88-49888AD819B0"
#define CHARACTERISTIC_UUID_TX "0A7EFBDE-7E87-4CDC-AF88-49888AD819B0"

extern HardwareSerial vescSerial;
extern bool deviceConnected; 

class BleBridge {
    public:
        BleBridge();
        void init();
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
          vescSerial.write(rxValue[i]);
        }
      }
    }
};

#endif