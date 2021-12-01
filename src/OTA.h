#ifndef __OTA_H__
#define __OTA_H__

#include "Arduino.h"
#include <NimBLEDevice.h>
#include "AppConfiguration.h"
#include "Buzzer.h"
#include "esp_ota_ops.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>
#include "base64.h"
#include "CanBus.h"

#define RESCUE_SERVICE_UUID                   "99EB1511-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_ID         "99EB1512-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_CONF       "99EB1513-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_FW         "99EB1514-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_HW_VERSION "99EB1515-A9E9-4024-B0A4-3DC4B4FABFB0"

#define SERVICE_UUID_OTA                      "c8659210-af91-4ad3-a995-a58d6fd26145" // UART service UUID

#define FULL_PACKET 512

class OTAUpdater {
    public:
      OTAUpdater();
      void setup();
      bool begin(const char* localName);
      NimBLEUUID getConfCharacteristicsUuid();

    private:
      BLEServer *pServer = NULL;

      BLEService *pESPOTAService = NULL;
      BLECharacteristic *pESPOTAIdCharacteristic = NULL;
      BLECharacteristic *pESPOTAConfCharacteristic = NULL;

      BLEService *pService = NULL;
      BLECharacteristic *pVersionCharacteristic = NULL;
      BLECharacteristic *pOtaCharacteristic = NULL;
    };

class BLECustomServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      // deviceConnected = true;
      // // code here for when device connects
    };

    void onDisconnect(BLEServer* pServer) {
      // deviceConnected = false;
      Buzzer::getInstance()->playSound(RTTTL_MELODIES::SIMPLE_BEEP_SCALE_DOWN);
      while (Buzzer::getInstance()->isPlayingSound()) {
        ;
      }
      ESP.restart();
    }
};

class OTACallback: public BLECharacteristicCallbacks {
  public:
    OTACallback(OTAUpdater* ble) {
      _p_ble = ble;
    }
    OTAUpdater* _p_ble;

    void onWrite(BLECharacteristic *pCharacteristic);
};

#endif
