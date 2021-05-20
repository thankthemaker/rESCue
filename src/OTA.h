#ifndef __OTA_H__
#define __OTA_H__

#include "Arduino.h"
#include <NimBLEDevice.h>
#include "AppConfiguration.h"
#include "Buzzer.h"
#include "esp_ota_ops.h"

#define SERVICE_UUID_ESPOTA             "d804b643-6ce7-4e81-9f8a-ce0f699085eb"
#define CHARACTERISTIC_UUID_ID          "d804b644-6ce7-4e81-9f8a-ce0f699085eb"
#define CHARACTERISTIC_UUID_CONF        "d804b645-6ce7-4e81-9f8a-ce0f699085eb"

#define SERVICE_UUID_OTA                "c8659210-af91-4ad3-a995-a58d6fd26145" // UART service UUID
#define CHARACTERISTIC_UUID_FW          "c8659211-af91-4ad3-a995-a58d6fd26145"
#define CHARACTERISTIC_UUID_HW_VERSION  "c8659212-af91-4ad3-a995-a58d6fd26145"

#define FULL_PACKET 512
#define CHARPOS_UPDATE_FLAG 5


class OTAUpdater {
    public:
      OTAUpdater();
      void setup();
      bool begin(const char* localName);
      NimBLEUUID getConfCharacteristicsUuid();
    
    private:
      String local_name;

      BLEServer *pServer = NULL;

      BLEService *pESPOTAService = NULL;
      BLECharacteristic * pESPOTAIdCharacteristic = NULL;
      BLECharacteristic * pESPOTAConfCharacteristic = NULL;

      BLEService *pService = NULL;
      BLECharacteristic * pVersionCharacteristic = NULL;
      BLECharacteristic * pOtaCharacteristic = NULL;
};

class BLECustomServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      // deviceConnected = true;
      // // code here for when device connects
    };

    void onDisconnect(BLEServer* pServer) {
      // deviceConnected = false;
      ////AppConfiguration::getInstance()->config.otaUpdateActive = 0;
      ////AppConfiguration::getInstance()->savePreferences();
      Buzzer::getInstance()->playSound(RTTTL_MELODIES::SIMPLE_BEEP_SCALE_DOWN);
      while (Buzzer::getInstance()->isPlayingSound()) {
        Serial.print(".");
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
