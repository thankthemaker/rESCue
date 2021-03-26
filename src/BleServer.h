#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#include "BleUartBridge.h"
#include "BleBlynkApp.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

extern bool deviceConnected; 

class BLEServer;
class BleUartBridge;
class BleBlynkApp;

class BleServer : public BLEServerCallbacks {
    public:
      BleServer();
      void init();
      BLEServer *pServer;
      BleUartBridge *bleUartBridge;
      BleBlynkApp *bleBlynkApp;

    void onConnect(BLEServer* pServer) {
#if DEBUG > 0
      Serial.println("BLE device connected");
#endif
      deviceConnected = true;
      ////bleBlynkApp->onConnect();
    }

    void onDisconnect(BLEServer* pServer) {
#if DEBUG > 0
     Serial.println("BLE device disconnected");
#endif
     deviceConnected = false;
     ////bleBlynkApp->onDisconnect();
    }
};
#endif