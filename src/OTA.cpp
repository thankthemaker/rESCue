#include "OTA.h"
#include "config.h"

esp_ota_handle_t otaHandler = 0;
AsyncWebServer server(80);

boolean wifiActive = false;
const char *wifiPassword;
bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;
uint32_t frameNumber = 0;

void startUpdate() {
  Serial.println("\nBeginOTA");
  const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
  Serial.println("Selected OTA partiton:");
  Serial.println("partition label:" + String(partition->label));
  Serial.println("partition size:" + String(partition->size));
  esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandler);
  updateFlag = true;
}

void handleUpdate(std::string data) {
/*
  Serial.printf("\nhandleUpdate incoming data (size %d):\n", data.length());
  for(int i=0; i<data.length();i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
*/
  esp_ota_write(otaHandler, data.c_str(), data.length());
  if (data.length() != FULL_PACKET) {
    esp_ota_end(otaHandler);
    Serial.println("\nEndOTA");
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (ESP_OK == esp_ota_set_boot_partition(partition)) {
      Serial.println("Activate partiton:");
      Serial.println("partition label:" + String(partition->label));
      Serial.println("partition size:" + String(partition->size));
      AppConfiguration::getInstance()->config.otaUpdateActive = 0;
      AppConfiguration::getInstance()->savePreferences();
      delay(2000);
      esp_restart();
    } else {
      Serial.println("Upload Error");
    }
  }
}

void activateWiFiAp(const char *password) {
  WiFi.softAP("rESCue OTA Updates", password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "alive");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost() && p->name().compareTo("data") != -1) {
        const char *value =p->value().c_str();
        Serial.printf("\nPOST[%s]: bytes %d\n", p->name().c_str(), p->value().length());
        if(!updateFlag) {
          startUpdate();
        } 
        if(p->value().length() > 0) {
          handleUpdate(base64_decode(value, false));
        }
      } 
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "ok");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  server.begin();
  wifiActive = true;
}

OTAUpdater::OTAUpdater() {
}

void OTAUpdater::setup() {
  begin("rESCue OTA Updates");
  Buzzer::getInstance()->playSound(RTTTL_MELODIES::SIMPLE_BEEP_SCALE_UP);
  while (Buzzer::getInstance()->isPlayingSound()) {
    ;
  }

	// Get Partitionsizes
	size_t ul;
	esp_partition_iterator_t _mypartiterator;
	const esp_partition_t *_mypart;
	ul = spi_flash_get_chip_size();
	Serial.print("\nFlash chip size: ");
	Serial.println(ul);
	Serial.println("Partition table:");
	_mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);

  while (_mypartiterator != NULL) {
		_mypart = esp_partition_get(_mypartiterator);
		printf("Type: %02x SubType %02x Address 0x%06X Size 0x%06X Encryption %i Label %s\r\n", _mypart->type, _mypart->subtype, _mypart->address, _mypart->size, _mypart->encrypted, _mypart->label);
		_mypartiterator = esp_partition_next(_mypartiterator);
	}

  _mypart = esp_ota_get_boot_partition();
  printf("Current active partition is %s\r\n", _mypart->label);
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
      std::string::size_type middle = str.find('='); // Find position of '='
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
      if(key == "wifiPassword") {
        wifiPassword = value.c_str();
      }
      if(key == "wifiActive") {
        if(value.compare("true") != -1) {
          activateWiFiAp(wifiPassword);
        }
      }
      if(key == "save") {
        AppConfiguration::getInstance()->config.otaUpdateActive = false;
        AppConfiguration::getInstance()->savePreferences();
        delay(100);
    }
  } else {
    if (!updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
      startUpdate();
    }
    if (_p_ble != NULL) {
      if (rxData.length() > 0) {
        Serial.print("Got frame " + String(frameNumber) + ", Bytes " + String(rxData.length()));
        handleUpdate(rxData);    
      }
    }

    delay(5); // needed to give BLE stack some time
    uint8_t txdata[4] = { (uint8_t)(frameNumber >> 24), (uint8_t)(frameNumber >> 16), (uint8_t)(frameNumber >> 8), (uint8_t)frameNumber};
    pCharacteristic->setValue((uint8_t *) txdata, 4);
    pCharacteristic->notify();
    Serial.println(", Ack. frame no. " + String(frameNumber++));
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
