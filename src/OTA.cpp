#include "OTA.h"
#include "config.h"

esp_ota_handle_t otaHandler = 0;

bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;

OTAUpdater::OTAUpdater() {
}

void OTAUpdater::setup() {
  begin("rESCue OTA Updates");
  Buzzer::getInstance()->playSound(RTTTL_MELODIES::SIMPLE_BEEP_SCALE_UP);
  while (Buzzer::getInstance()->isPlayingSound()) {
    Serial.print(".");
  }
}

NimBLEUUID OTAUpdater::getConfCharacteristicsUuid() {
  return pESPOTAConfCharacteristic->getUUID();
}

void OTACallback::onWrite(BLECharacteristic *pCharacteristic) {
  std::string rxData = pCharacteristic->getValue();

  if(pCharacteristic->getUUID().equals(_p_ble->getConfCharacteristicsUuid())) {
      //Serial.println("Write config");
      //Serial.println("String " + String(rxData.c_str()));
      //Serial.println("Length " + String(rxData.length()));
      
      std::string str(rxData.c_str());
      std::string::size_type middle = str.find('='); // Find position of ','
      std::string key = "";
      std::string value = "";      
      if(middle != std::string::npos) {
        key = str.substr(0, middle);
        value = str.substr(middle + 1, str.size() - (middle + 1));
      }

      Serial.println(String(key.c_str()) + String("=") + String(value.c_str()));

      if(key == "numberPixelLight") {
        AppConfiguration::getInstance()->config.numberPixelLight = atoi(value.c_str());
      }
      if(key == "numberPixelBatMon") {
        AppConfiguration::getInstance()->config.numberPixelBatMon = atoi(value.c_str());
      }
      if(key == "vescId") {
        AppConfiguration::getInstance()->config.vescId = atoi(value.c_str());
      } 
      if(key == "authToken") {
        AppConfiguration::getInstance()->config.authToken = value.c_str();
      }
      if(key == "save") {
        AppConfiguration::getInstance()->config.otaUpdateActive = false;
        AppConfiguration::getInstance()->savePreferences();
        delay(100);
    }
  } else {
    if (!updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
      Serial.println("BeginOTA");
      const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
      Serial.println("partition label:" + String(partition->label));
      Serial.println("partition size:" + String(partition->size));
      esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandler);
      updateFlag = true;
    }
    if (_p_ble != NULL) {
      if (rxData.length() > 0) {
        Serial.print(".");
        esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
        if (rxData.length() != FULL_PACKET) {
          esp_ota_end(otaHandler);
          Serial.println("EndOTA");
          AppConfiguration::getInstance()->config.otaUpdateActive = 0;
          AppConfiguration::getInstance()->savePreferences();
          if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
            delay(2000);
            esp_restart();
          } else {
            Serial.println("Upload Error");
          }
        }
      }
    }

    uint8_t txData[5] = {1, 2, 3, 4, 5};
    //delay(1000);
    pCharacteristic->setValue((uint8_t*)txData, 5);
    pCharacteristic->notify();
  }
}

bool OTAUpdater::begin(const char* localName = "UART Service") {
  // Create the BLE Device
  BLEDevice::init(localName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BLECustomServerCallbacks());

  // Create the BLE Service
  pESPOTAService = pServer->createService(SERVICE_UUID_ESPOTA);
  pService = pServer->createService(SERVICE_UUID_OTA);

  // Create a BLE Characteristic
  pESPOTAIdCharacteristic = pESPOTAService->createCharacteristic(
                                       CHARACTERISTIC_UUID_ID,
                                       NIMBLE_PROPERTY::READ
                                     );

  pESPOTAConfCharacteristic = pESPOTAService->createCharacteristic(
                         CHARACTERISTIC_UUID_CONF,
                         NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
                       );
  pVersionCharacteristic = pService->createCharacteristic(
                             CHARACTERISTIC_UUID_HW_VERSION,
                            NIMBLE_PROPERTY::READ
                           );

  pOtaCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID_FW,
                         NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
                       );

  pESPOTAConfCharacteristic->setCallbacks(new OTACallback(this));
  pOtaCharacteristic->setCallbacks(new OTACallback(this));

  // Start the service(s)
  pESPOTAService->start();
  pService->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID_ESPOTA);
  pServer->getAdvertising()->start();

  uint8_t hardwareVersion[5] = {HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR, SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH};
  pVersionCharacteristic->setValue((uint8_t*)hardwareVersion, 5);
  return true;
}