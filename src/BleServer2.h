#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#include "config.h"

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <utility/BlynkFifo.h>
#include <NimBLEDevice.h>

class NimBLEServer;

extern bool deviceConnected;
extern BlynkFifo<uint8_t, BLYNK_MAX_READBYTES*2> mBuffRX;
extern Stream* vescSerial;

#define BLYNK_PRINT Serial
#define BLYNK_USE_DIRECT_CONNECT

#define VESC_SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define VESC_CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define VESC_CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLYNK_SERVICE_UUID           "713D0000-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_RX "713D0003-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_TX "713D0002-503E-4C75-BA94-3148F18D941E"


class BleServer : public NimBLEServerCallbacks {
    public:
      BleServer();
      void init(Stream *vesc);
      void loop();
      void loopUart();

      void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
      void onDisconnect(NimBLEServer* pServer);
      void setDeviceName(const char* name);
      void begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p);
      void begin();
      bool connect();
      void disconnect();
      bool connected();
      size_t read(void* buf, size_t len);
      size_t write(const void* buf, size_t len);
      size_t available();

      NimBLEServer *pServer;

    private:
      boolean initialized = false;
};

class BleBlynkCallbacks : public NimBLECharacteristicCallbacks {
      void onWrite(NimBLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        uint8_t* data = (uint8_t*)rxValue.data();
        size_t len = rxValue.length();

        BLYNK_DBG_DUMP(">> ", data, len);
        mBuffRX.put(data, len);
      }
    }
};

class BleUartCallbacks: public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic) {
      Serial.print(F("\nBLE-write from phone: "));
    }

    void onWrite(NimBLECharacteristic* pCharacteristic) {
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

class BlynkEsp32_BLE : public BlynkProtocol<BleServer> {
    typedef BlynkProtocol<BleServer> Base;
    public:
      BlynkEsp32_BLE(BleServer& transp) : Base(transp) {}

      void begin(const char* auth) {
        Base::begin(auth);
        state = DISCONNECTED;
        conn.begin();
      }

      void setDeviceName(const char* name) {
        conn.setDeviceName(name);
      }
};

#endif