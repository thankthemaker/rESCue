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

#define VESC_SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define VESC_CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define VESC_CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLYNK_SERVICE_UUID           "713D0000-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_RX "713D0003-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_TX "713D0002-503E-4C75-BA94-3148F18D941E"

extern Stream* vescSerial;

class BleUartBridge {
    public:
        BleUartBridge(BLEServer *pServer);
        void init(Stream *vesc);
        void loop();
        BLEServer *pServer;
        BLECharacteristic *pTxCharacteristicUart; 
        bool deviceConnected;
        bool oldDeviceConnected;
};

class BleUartCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
      Serial.print(F("\nBLE-write from phone: "));
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

#if DEBUG > 2
      unsigned char buffer[rxValue.length()];
      memcpy(buffer, rxValue.data(), rxValue.length());
      Serial.print(F("\nBLE-write from phone: "));
     for (int i = 0; i < rxValue.length(); i++) {
       Serial.print((int)rxValue.data()[i], HEX);
     }
#endif

      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          vescSerial->write(rxValue[i]);
        }
      }
    }
};

#endif //ESP32
#endif //__BLUETOOTH_BRIDGE_H__