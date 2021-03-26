#ifndef __BLYNKAPP_H__
#define __BLYNKAPP_H__

#include "Arduino.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "BleBlynkCallbacks.h"

#define BLYNK_PRINT Serial
#define BLYNK_USE_DIRECT_CONNECT

class BleBlynkApp {
    public:
        BleBlynkApp();  // ToDo does not work
        BleBlynkApp(BLEServer *pServer);
        void init();
        void loop();

        void onConnect();
        void onDisconnect();
        void setDeviceName(const char* name);
        void begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p);
        void begin();
        bool connect();
        void disconnect();
        bool connected();
        size_t read(void* buf, size_t len);
        size_t write(const void* buf, size_t len);
        size_t available();

        BLEServer *pServer;
        BLECharacteristic *pTxCharacteristicBlynk; 
        bool deviceConnected;
        bool oldDeviceConnected;
};

class BlynkEsp32_BLE : public BlynkProtocol<BleBlynkApp> {
    typedef BlynkProtocol<BleBlynkApp> Base;
    public:
      BlynkEsp32_BLE(BleBlynkApp& transp) : Base(transp) {}

      void begin(const char* auth) {
        Base::begin(auth);
        state = DISCONNECTED;
        conn.begin();
      }

      void setDeviceName(const char* name) {
        conn.setDeviceName(name);
      }
};

static BleBlynkApp _blynkTransportBLE;
BlynkEsp32_BLE Blynk(_blynkTransportBLE);

#endif