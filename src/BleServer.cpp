#include "BleServer.h"
#include "BlynkPins.h"
#include <Logger.h>
#include <sstream>
#include "esp_bt_main.h"
#include "esp_ota_ops.h"
#include <ESPAsyncWebServer.h>

#define FULL_PACKET 512

int MTU_SIZE = FULL_PACKET;
int BLE_PACKET_SIZE = MTU_SIZE - 3;
NimBLEServer *pServer = nullptr;
NimBLEService *pServiceVesc = nullptr;
NimBLEService *pServiceRescue = nullptr;
NimBLECharacteristic *pCharacteristicVescTx = nullptr;
NimBLECharacteristic *pCharacteristicVescRx = nullptr;
NimBLECharacteristic *pCharacteristicConf = nullptr;
NimBLECharacteristic *pOtaCharacteristic = nullptr;
NimBLECharacteristic *pCharacteristicId = nullptr;
NimBLECharacteristic *pCharacteristicVersion = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
Stream *vescSerial;
std::string bufferString;
int bleLoop = 0;

BleServer::BleServer() {}

esp_ota_handle_t otaHandler = 0;
bool updateFlag = false;
uint32_t frameNumber = 0;
AsyncWebServer server(80);
boolean wifiActive = false;
const char *wifiPassword;

void startUpdate() {
    Serial.println("\nBeginOTA");
    const esp_partition_t *partition = esp_ota_get_next_update_partition(nullptr);
    Serial.println("Selected OTA partiton:");
    Serial.println("partition label:" + String(partition->label));
    Serial.println("partition size:" + String(partition->size));
    esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandler);
    updateFlag = true;
    Serial.println("Update started");
}

void handleUpdate(const std::string &data) {
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
        const esp_partition_t *partition = esp_ota_get_next_update_partition(nullptr);
        if (ESP_OK == esp_ota_set_boot_partition(partition)) {
            Serial.println("Activate partiton:");
            Serial.println("partition label:" + String(partition->label));
            Serial.println("partition size:" + String(partition->size));
            AppConfiguration::getInstance()->config.otaUpdateActive = false;
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
    server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "alive");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        int params = request->params();
        for (int i = 0; i < params; i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isPost() && p->name().compareTo("data") != -1) {
                const char *value = p->value().c_str();
                //Serial.printf("\nPOST[%s]: bytes %d\n", p->name().c_str(), p->value().length());
                if (!updateFlag) {
                    startUpdate();
                }
                if (p->value().length() > 0) {
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

// NimBLEServerCallbacks::onConnect
inline
void BleServer::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    char buf[128];
    snprintf(buf, 128, "Client connected: %s",  NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
    Logger::notice(LOG_TAG_BLESERVER, "Multi-connect support: start advertising");
    deviceConnected = true;
    NimBLEDevice::startAdvertising();
};

// NimBLEServerCallbacks::onDisconnect
inline
void BleServer::onDisconnect(NimBLEServer *pServer) {
    Logger::notice(LOG_TAG_BLESERVER, "Client disconnected - start advertising");
    deviceConnected = false;
    NimBLEDevice::startAdvertising();
}

// NimBLEServerCallbacks::onMTUChange
inline
void BleServer::onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
    char buf[128];
    snprintf(buf, 128, "MTU changed - new size %d, peer %s", MTU, NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
    MTU_SIZE = MTU;
    BLE_PACKET_SIZE = MTU_SIZE - 3;
}

void BleServer::init(Stream *vesc, CanBus *canbus) {
    vescSerial = vesc;

    // Create the BLE Device
    NimBLEDevice::init(AppConfiguration::getInstance()->config.deviceName.c_str());
    int mtu_size = AppConfiguration::getInstance()->config.mtuSize;
    NimBLEDevice::setMTU(mtu_size);
    char buf[128];
    snprintf(buf, 128, "Initial MTU size %d", mtu_size);
    Logger::notice(LOG_TAG_BLESERVER, buf);
    this->canbus = canbus;

    // Create the BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);
    auto pSecurity = new NimBLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    // Create the VESC BLE Service
    pServiceVesc = pServer->createService(VESC_SERVICE_UUID);
    // Create the RESCUE BLE Service
    pServiceRescue = pServer->createService(RESCUE_SERVICE_UUID);

    // Create a BLE Characteristic for VESC TX
    pCharacteristicVescTx = pServiceVesc->createCharacteristic(
            VESC_CHARACTERISTIC_UUID_TX,
            NIMBLE_PROPERTY::NOTIFY |
            NIMBLE_PROPERTY::READ
    );
    //pCharacteristicVescTx->setValue("VESC TX");
    pCharacteristicVescTx->setCallbacks(this);

    // Create a BLE Characteristic for VESC RX
    pCharacteristicVescRx = pServiceVesc->createCharacteristic(
            VESC_CHARACTERISTIC_UUID_RX,
            NIMBLE_PROPERTY::WRITE
    );
    //pCharacteristicVescRx->setValue("VESC RX");
    pCharacteristicVescRx->setCallbacks(this);

    // Create a BLE Characteristic
    pCharacteristicId = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_ID,
            NIMBLE_PROPERTY::READ
    );
    pCharacteristicId->setCallbacks(this);

    pCharacteristicConf = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_CONF,
            NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
    );
    pCharacteristicConf->setCallbacks(this);

    pOtaCharacteristic = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_FW,
            NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
    );

    pOtaCharacteristic->setCallbacks(this);

    uint8_t hardwareVersion[5] = {HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR, SOFTWARE_VERSION_MAJOR,
                                  SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH};

    pCharacteristicVersion = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_HW_VERSION,
            NIMBLE_PROPERTY::READ
    );
    pCharacteristicVersion->setValue((uint8_t *) hardwareVersion, 5);
    pCharacteristicVersion->setCallbacks(this);

    // Start the VESC service
    pServiceVesc->start();
    pServiceRescue->start();

    // Start advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(VESC_SERVICE_UUID);
    pAdvertising->addServiceUUID(RESCUE_SERVICE_UUID);
    pAdvertising->setAppearance(0x00);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

    pAdvertising->start();
    Logger::notice(LOG_TAG_BLESERVER, "waiting a client connection to notify...");
}

#ifdef CANBUS_ENABLED

void BleServer::loop(CanBus::VescData *vescData, long loopTime, long maxLoopTime) {
#else
    void BleServer::loop() {
#endif
    if (vescSerial->available()) {
        int oneByte;
        while (vescSerial->available()) {
            oneByte = vescSerial->read();
            bufferString.push_back(oneByte);
        }

        if (deviceConnected) {
            while (bufferString.length() > 0) {
                if (bufferString.length() > BLE_PACKET_SIZE) {
                    dumpBuffer("VESC => BLE/UART", bufferString.substr(0, BLE_PACKET_SIZE));
                    pCharacteristicVescTx->setValue(bufferString.substr(0, BLE_PACKET_SIZE));
                    bufferString = bufferString.substr(BLE_PACKET_SIZE);
                } else {
                    dumpBuffer("VESC => BLE/UART", bufferString);
                    pCharacteristicVescTx->setValue(bufferString);
                    bufferString.clear();
                }
                pCharacteristicVescTx->notify();
                delay(10); // bluetooth stack will go into congestion, if too many packets are sent
            }
        }
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Logger::notice(LOG_TAG_BLESERVER, "start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

#ifdef CANBUS_ENABLED
    if (millis() - bleLoop > 500) {
        updateRescueApp(loopTime, maxLoopTime);
        bleLoop = millis();
    }
#endif //CANBUS_ENABLED
}

void BleServer::dumpBuffer(std::string header, std::string buffer) {
    if(!Logger::getLogLevel() == Logger::VERBOSE) {
        return;
    }
    char tmpbuf[1024];
    int length = snprintf(tmpbuf, 50, "%s : len = %d / ", header.c_str(), buffer.length());
    for (int i = 0; i < buffer.size(); i++) {
        length += snprintf(tmpbuf+length, 1024-length, "%02x ", buffer.at(i));
    }
    Logger::verbose(LOG_TAG_BLESERVER, tmpbuf);
}

//NimBLECharacteristicCallbacks::onWrite
void BleServer::onWrite(BLECharacteristic *pCharacteristic) {
    char buffer[128];
    snprintf(buffer, 128, "onWrite to characteristics: %s", pCharacteristic->getUUID().toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buffer);
    std::string rxValue = pCharacteristic->getValue();
    Logger::notice(LOG_TAG_BLESERVER, buffer);
    if (rxValue.length() > 0) {
        if (pCharacteristic->getUUID().equals(pCharacteristicVescRx->getUUID())) {
            dumpBuffer("BLE/UART => VESC: ", rxValue);

#ifdef CANBUS_ONLY
            canbus->proxy->proxyIn(rxValue);
#else
            for (int i = 0; i < rxValue.length(); i++) {
              vescSerial->write(rxValue[i]);
            }
#endif
        } else if (pCharacteristic->getUUID().equals(pCharacteristicConf->getUUID())) {
            char buf[128];
            std::string str(rxValue.c_str());
            std::string::size_type middle = str.find('='); // Find position of '='
            std::string key = "";
            std::string value = "";
            if (middle != std::string::npos) {
                key = str.substr(0, middle);
                value = str.substr(middle + 1, str.size() - (middle + 1));
            }

            Serial.println(String(key.c_str()) + String("=") + String(value.c_str()));

            if (key == "config") {
                AppConfiguration::getInstance()->config.sendConfig = true;
            } else if (key == "save") {
                AppConfiguration::getInstance()->config.otaUpdateActive = false;
                AppConfiguration::getInstance()->config.saveConfig = true;
                delay(100);
            } else if (key == "deviceName") {
                AppConfiguration::getInstance()->config.deviceName = value.c_str();
            } else if (key == "isNotificationEnabled") {
                AppConfiguration::getInstance()->config.isNotificationEnabled = value.c_str();
            } else if (key == "minBatteryVoltage") {
                AppConfiguration::getInstance()->config.minBatteryVoltage = atof(value.c_str());
            } else if (key == "lowBatteryVoltage") {
                AppConfiguration::getInstance()->config.lowBatteryVoltage = atof(value.c_str());
            } else if (key == "maxBatteryVoltage") {
                AppConfiguration::getInstance()->config.maxBatteryVoltage = atof(value.c_str());
            } else if (key == "batteryDrift") {
                AppConfiguration::getInstance()->config.batteryDrift = atof(value.c_str());
            } else if (key == "startSoundIndex") {
                AppConfiguration::getInstance()->config.startSoundIndex = atoi(value.c_str());
                Buzzer::getInstance()->stopSound();
                Buzzer::getInstance()->playSound(RTTTL_MELODIES(atoi(value.c_str())));
                snprintf(buf, 128, "Updated param \"StartSoundIndex\" to %s", value.c_str());
            } else if (key == "startLightIndex") {
                AppConfiguration::getInstance()->config.startLightIndex = atoi(value.c_str());
            } else if (key == "batteryWarningSoundIndex") {
                AppConfiguration::getInstance()->config.batteryWarningSoundIndex = atoi(value.c_str());
                Buzzer::getInstance()->stopSound();
                Buzzer::getInstance()->playSound(RTTTL_MELODIES(atoi(value.c_str())));
                snprintf(buf, 128, "Updated param \"BatteryWarningSoundIndex\" to %s", value.c_str());
            } else if (key == "batteryAlarmSoundIndex") {
                AppConfiguration::getInstance()->config.batteryAlarmSoundIndex = atoi(value.c_str());
                Buzzer::getInstance()->stopSound();
                Buzzer::getInstance()->playSound(RTTTL_MELODIES(atoi(value.c_str())));
                snprintf(buf, 128, "Updated param \"BatteryAlarmSoundIndex\" to %s", value.c_str());
            } else if (key == "startLightDuration") {
                AppConfiguration::getInstance()->config.startLightDuration = atoi(value.c_str());
            } else if (key == "idleLightIndex") {
                AppConfiguration::getInstance()->config.idleLightIndex = atoi(value.c_str());
            } else if (key == "idleLightTimeout") {
                AppConfiguration::getInstance()->config.idleLightTimeout = atoi(value.c_str());
            } else if (key == "lightFadingDuration") {
                AppConfiguration::getInstance()->config.lightFadingDuration = atoi(value.c_str());
            } else if (key == "lightMaxBrightness") {
                AppConfiguration::getInstance()->config.lightMaxBrightness = atoi(value.c_str());
            } else if (key == "lightColorPrimary") {
                AppConfiguration::getInstance()->config.lightColorPrimary = atoi(value.c_str());
                snprintf(buf, 128, "Updated param \"lightColorPrimary\" to %s (%d)", value.c_str(),
                         AppConfiguration::getInstance()->config.lightColorPrimary);
            } else if (key == "lightColorSecondary") {
                AppConfiguration::getInstance()->config.lightColorSecondary = atoi(value.c_str());
                snprintf(buf, 128, "Updated param \"lightColorSecondary\" to %s (%d", value.c_str(),
                         AppConfiguration::getInstance()->config.lightColorSecondary);
            } else if (key == "brakeLightEnabled") {
                AppConfiguration::getInstance()->config.brakeLightEnabled = atoi(value.c_str());
            } else if (key == "brakeLightMinAmp") {
                AppConfiguration::getInstance()->config.brakeLightMinAmp = atoi(value.c_str());
            } else if (key == "numberPixelLight") {
                AppConfiguration::getInstance()->config.numberPixelLight = atoi(value.c_str());
            } else if (key == "numberPixelBatMon") {
                AppConfiguration::getInstance()->config.numberPixelBatMon = atoi(value.c_str());
            } else if (key == "vescId") {
                AppConfiguration::getInstance()->config.vescId = atoi(value.c_str());
            } else if (key == "authToken") {
                AppConfiguration::getInstance()->config.authToken = value.c_str();
            } else if (key == "ledType") {
                AppConfiguration::getInstance()->config.ledType = value.c_str();
            } else if (key == "ledFrequency") {
                AppConfiguration::getInstance()->config.ledFrequency = value.c_str();
            } else if (key == "logLevel") {
                AppConfiguration::getInstance()->config.logLevel = static_cast<Logger::Level>(atoi(value.c_str()));
            } else if (key == "mtuSize") {
                uint16_t mtu = atoi(value.c_str());
                AppConfiguration::getInstance()->config.mtuSize = mtu;
                if (mtu != 0 && mtu < MTU_SIZE) {
                    NimBLEDevice::setMTU(mtu);
                    snprintf(buf, 128, "New MTU-size: %d", mtu);
                    Logger::warning(LOG_TAG_BLESERVER, buf);
                }
            } else if (key == "wifiActive") {
                if (value.compare("true") != -1) {
                    activateWiFiAp(wifiPassword);
                }
            } else if (key == "wifiPassword") {
                wifiPassword = value.c_str();
            }
            Logger::notice(LOG_TAG_BLESERVER, buf);
        } else if (pCharacteristic->getUUID().equals(pOtaCharacteristic->getUUID())) {
            if (!updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
                startUpdate();
            }

            Serial.print("Got frame " + String(frameNumber) + ", Bytes " + String(rxValue.length()));
            handleUpdate(rxValue);

            delay(5); // needed to give BLE stack some time
            uint8_t txdata[4] = {(uint8_t) (frameNumber >> 24), (uint8_t) (frameNumber >> 16),
                                 (uint8_t) (frameNumber >> 8),
                                 (uint8_t) frameNumber};
            pCharacteristic->setValue((uint8_t *) txdata, 4);
            pCharacteristic->notify();
            Serial.println(", Ack. frame no. " + String(frameNumber++));
        }
    }
}

//NimBLECharacteristicCallbacks::onSubscribe
void BleServer::onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue) {
    char buf[256];
    snprintf(buf, 256, "Client ID: %d, Address: %s, Subvalue %d, Characteristics %s ",
             desc->conn_handle, NimBLEAddress(desc->peer_ota_addr).toString().c_str(), subValue,
             pCharacteristic->getUUID().toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
}

//NimBLECharacteristicCallbacks::onSubscribe
void BleServer::onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code) {
    if(Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[256];
        snprintf(buf, 256, "Notification/Indication status code: %d, return code: %d", status, code);
        Logger::verbose(LOG_TAG_BLESERVER, buf);
    }
}

#ifdef CANBUS_ENABLED

void BleServer::updateRescueApp(long loopTime, long maxLoopTime) {
    this->sendValue("loopTime", loopTime);
    this->sendValue("maxLoopTime", maxLoopTime);

/*
   Blynk.setProperty(VPIN_VESC_DUTY_CYCLE, "color",
     vescData->dutyCycle > 0 ? BLINK_COLOR_GREEN : BLINK_COLOR_RED);
   if(AppConfiguration::getInstance()->config.isNotificationEnabled){
     if(millis() - lastNotification > 60000) { // Notification only all once a minutes
       if(vescData->inputVoltage > AppConfiguration::getInstance()->config.maxBatteryVoltage) {
         Blynk.notify("Battery too high: " + String(vescData->inputVoltage) + "V");
       }
       if(vescData->inputVoltage < AppConfiguration::getInstance()->config.minBatteryVoltage) {
         Blynk.notify("Battery dropped below: " + String(vescData->inputVoltage) + "V");
       }  
       lastBatteryValue = vescData->inputVoltage;
       lastNotification = millis();
     }
   }
*/
}

template<typename TYPE>
void BleServer::sendValue(std::string key, TYPE value) {
    std::stringstream ss;
    ss << key << "=" << value;
    pCharacteristicConf->setValue(ss.str());
    pCharacteristicConf->notify();
    ss.str("");
    delay(5);
}

boolean isStringType(String a) { return true; }

boolean isStringType(std::string a) { return true; }

boolean isStringType(int a) { return false; }

boolean isStringType(double a) { return false; }

boolean isStringType(boolean a) { return false; }

struct BleServer::sendConfigValue {
    NimBLECharacteristic *pCharacteristic;
    std::stringstream ss;

    sendConfigValue(NimBLECharacteristic *pCharacteristic) {
        this->pCharacteristic = pCharacteristic;
    }

    template<typename T>
    void operator()(const char *name, const T &value) {
        if (isStringType(value)) {
            ss << name << "=" << static_cast<String>(value).c_str();
        } else {
            ss << name << "=" << value;
        }
        Serial.println("Sending: " + String(ss.str().c_str()));
        pCharacteristic->setValue(ss.str());
        pCharacteristic->indicate();
        ss.str("");
        delay(5);
    }
};

void BleServer::sendConfig() {
    visit_struct::for_each(AppConfiguration::getInstance()->config, sendConfigValue(pCharacteristicConf));
}

#endif //CANBUS_ENABLED