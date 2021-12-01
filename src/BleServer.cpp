#include "BleServer.h"
#include "BlynkPins.h"
#include <Logger.h>
#include <sstream>
#include "esp_bt_main.h"

const int BLE_PACKET_SIZE = 128;
NimBLEServer *pServer = NULL;
NimBLEService *pServiceVesc = NULL;
NimBLEService *pServiceRescue = NULL;
NimBLECharacteristic *pCharacteristicVescTx = NULL;
NimBLECharacteristic *pCharacteristicVescRx = NULL;
NimBLECharacteristic *pCharacteristicConf = NULL;
NimBLECharacteristic *pCharacteristicId = NULL;
NimBLECharacteristic *pCharacteristicVersion = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
Stream *vescSerial;
std::string bufferString;
int bleLoop = 0;

BleServer::BleServer() {}

// NimBLEServerCallbacks::onConnect
inline
void BleServer::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    char buf[128];
    snprintf(buf, 128, "Client connected: %s", NimBLEAddress(desc->peer_ota_addr).toString().c_str());
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

void BleServer::init(Stream *vesc, CanBus *canbus) {
    vescSerial = vesc;

    // Create the BLE Device
    NimBLEDevice::init(BT_NAME);

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

    uint8_t hardwareVersion[5] = {HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR, SOFTWARE_VERSION_MAJOR,
                                  SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH};

    pCharacteristicVersion = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_HW_VERSION,
            NIMBLE_PROPERTY::READ
    );
    pCharacteristicVersion->setValue((uint8_t *) hardwareVersion, 5);

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
        //if (Logger::getLogLevel() == Logger::VERBOSE) {
        Logger::notice(LOG_TAG_BLESERVER, "BLE from VESC: ");
        Logger::notice(LOG_TAG_BLESERVER, bufferString.c_str());
        // }

        if (deviceConnected) {
            while (bufferString.length() > 0) {
                if (bufferString.length() > BLE_PACKET_SIZE) {
                    pCharacteristicVescTx->setValue(bufferString.substr(0, BLE_PACKET_SIZE));
                    bufferString = bufferString.substr(BLE_PACKET_SIZE);
                } else {
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
        updateRescueApp(vescData, loopTime, maxLoopTime);
        bleLoop = millis();
    }
#endif //CANBUS_ENABLED
}

//NimBLECharacteristicCallbacks::onWrite
void BleServer::onWrite(BLECharacteristic *pCharacteristic) {
    char buffer[128];
    snprintf(buffer, 128, "onWrite to characteristics: %s", pCharacteristic->getUUID().toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buffer);
    std::string rxValue = pCharacteristic->getValue();
    snprintf(buffer, 128, "onWrite - value: %s\n", rxValue.data());
    Logger::notice(LOG_TAG_BLESERVER, buffer);
    if (rxValue.length() > 0) {
        if (pCharacteristic->getUUID().equals(pCharacteristicVescRx->getUUID())) {
/*
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i], DEC);
        Serial.print(" ");
      }
      Serial.println();
*/

#ifdef CANBUS_ONLY
            canbus->proxyIn(rxValue);
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
                AppConfiguration::getInstance()->savePreferences();
                delay(100);
            } else if (key == "isNotificationEnabled") {
                AppConfiguration::getInstance()->config.isNotificationEnabled = value.c_str();
            } else if (key == "minBatteryVoltage") {
                AppConfiguration::getInstance()->config.minBatteryVoltage = atof(value.c_str());
            } else if (key == "maxBatteryVoltage") {
                AppConfiguration::getInstance()->config.maxBatteryVoltage = atof(value.c_str());
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
            } else if (key == "lightFadingDuration") {
                AppConfiguration::getInstance()->config.lightFadingDuration = atoi(value.c_str());
            } else if (key == "lightMaxBrightness") {
                AppConfiguration::getInstance()->config.lightMaxBrightness = atoi(value.c_str());
            } else if (key == "lightColorPrimary") {
                AppConfiguration::getInstance()->config.lightColorPrimary = atoi(value.c_str());
            } else if (key == "lightColorSecondary") {
                AppConfiguration::getInstance()->config.lightColorSecondary = atoi(value.c_str());
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
            }
            Logger::notice(LOG_TAG_BLESERVER, buf);
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
    char buf[256];
    snprintf(buf, 256, "Notification/Indication status code: %d, return code: %d", status, code);
    Logger::verbose(LOG_TAG_BLESERVER, buf);
}

#ifdef CANBUS_ENABLED

void BleServer::updateRescueApp(CanBus::VescData *vescData, long loopTime, long maxLoopTime) {
    this->sendValue("vesc.voltage", vescData->inputVoltage);
    this->sendValue("vesc.erpm", vescData->erpm);
    this->sendValue("vesc.dutyCycle", vescData->dutyCycle);
    this->sendValue("vesc.mosfetTemp", vescData->mosfetTemp);
    this->sendValue("vesc.motorTemp", vescData->motorTemp);
    this->sendValue("vesc.ampHours", vescData->ampHours);
    this->sendValue("vesc.ampHoursCharged", vescData->ampHoursCharged);
    this->sendValue("vesc.wattHours", vescData->wattHours);
    this->sendValue("vesc.wattHoursCharged", vescData->wattHoursCharged);
    this->sendValue("vesc.current", vescData->current);
    this->sendValue("vesc.tachometer", vescData->tachometer);
    this->sendValue("vesc.tachometerAbsolut", vescData->tachometerAbsolut);
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