#include "OTA.h"
#include "config.h"

esp_ota_handle_t otaHandler = 0;
WiFiServer server(80);

bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;
uint32_t frameNumber = 0;

OTAUpdater::OTAUpdater() {
}

void OTAUpdater::setup() {
  begin("rESCue OTA Updates");
  Buzzer::getInstance()->playSound(RTTTL_MELODIES::SIMPLE_BEEP_SCALE_UP);
  while (Buzzer::getInstance()->isPlayingSound()) {
    ;
  }

  WiFi.softAP("rESCue OTA Updates", "thankthemaker");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

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

void OTAUpdater::loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
      }
    }
    client.stop();
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
      Serial.println("\nBeginOTA");
      const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
      Serial.println("Selected OTA partiton:");
      Serial.println("partition label:" + String(partition->label));
      Serial.println("partition size:" + String(partition->size));
      esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandler);
      updateFlag = true;
    }
    if (_p_ble != NULL) {
      if (rxData.length() > 0) {
        Serial.print("Got frame " + String(frameNumber));
        esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
        if (rxData.length() != FULL_PACKET) {
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