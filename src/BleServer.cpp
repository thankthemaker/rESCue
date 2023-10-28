#include "BleServer.h"
#include <Logger.h>
#include <sstream>

int MTU_SIZE = 128;
int PACKET_SIZE = MTU_SIZE-3;
NimBLEServer *pServer = nullptr;
NimBLEService *pServiceVesc = nullptr;
NimBLEService *pServiceRescue = nullptr;
NimBLECharacteristic *pCharacteristicVescTx = nullptr;
NimBLECharacteristic *pCharacteristicVescRx = nullptr;
NimBLECharacteristic *pCharacteristicConf = nullptr;
NimBLECharacteristic *pCharacteristicFw = nullptr;
NimBLECharacteristic *pCharacteristicLoop = nullptr;
NimBLECharacteristic *pCharacteristicId = nullptr;
NimBLECharacteristic *pCharacteristicVersion = nullptr;
char tmpbuf[1024]; // CAUTION: always use a global buffer, local buffer will flood the stack


bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
Stream *vescSerial;
std::string vescBuffer;
std::string updateBuffer;
unsigned long bleLoop = 0;
unsigned long loopTimeSum = 0;
unsigned long loopCount = 0;
unsigned int bleWait = 5;

BleServer::BleServer() = default;

uint32_t frameNumber = 0;


// NimBLEServerCallbacks::onConnect
inline
void BleServer::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    snprintf(buf, bufSize, "Client connected: %s",  NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
    Logger::notice(LOG_TAG_BLESERVER, "Multi-connect support: start advertising");
    deviceConnected = true;
    NimBLEDevice::startAdvertising();
}

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
    snprintf(buf, bufSize, "MTU changed - new size %d, peer %s", MTU, NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
    MTU_SIZE = MTU;
    PACKET_SIZE = MTU_SIZE - 3;
}

#if defined(CANBUS_ENABLED)
void BleServer::init(Stream *vesc, CanBus *canbus) {
#else
void BleServer::init(Stream *vesc) {
#endif
    vescSerial = vesc;

    // Create the BLE Device
    NimBLEDevice::init(AppConfiguration::getInstance()->config.deviceName.c_str());
    int mtu_size = AppConfiguration::getInstance()->config.mtuSize;
    NimBLEDevice::setMTU(mtu_size);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    snprintf(buf, bufSize, "Initial MTU size %d", mtu_size);
    Logger::notice(LOG_TAG_BLESERVER, buf);
#if defined(CANBUS_ENABLED)
    this->canbus = canbus;
#endif
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
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR
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
            NIMBLE_PROPERTY::NOTIFY | 
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR
    );
    pCharacteristicConf->setCallbacks(this);

    pCharacteristicFw = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_FW,
            NIMBLE_PROPERTY::NOTIFY | 
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR
    );
    pCharacteristicConf->setCallbacks(this);

    pCharacteristicLoop = pServiceRescue->createCharacteristic(
            RESCUE_CHARACTERISTIC_UUID_LOOP,
            NIMBLE_PROPERTY::NOTIFY | 
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR
    );
    pCharacteristicLoop->setCallbacks(this);

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
//    pAdvertising->setAppearance(0x00);
//    pAdvertising->setScanResponse(true);
//    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

    pAdvertising->start();
    Logger::notice(LOG_TAG_BLESERVER, "waiting a client connection to notify...");
}

void BleServer::loop(VescData *vescData, unsigned long loopTime, unsigned long maxLoopTime) {
    loopCount++;
    loopTimeSum += loopTime;
    
    if (vescSerial->available()) {
        int oneByte;
        while (vescSerial->available()) {
            oneByte = vescSerial->read();
            vescBuffer.push_back(oneByte);
        }

        if (deviceConnected) {
            while (vescBuffer.length() > 0) {
                if (vescBuffer.length() > PACKET_SIZE) {
                    dumpBuffer("VESC => BLE/UART", vescBuffer.substr(0, PACKET_SIZE));
                    pCharacteristicVescTx->setValue(vescBuffer.substr(0, PACKET_SIZE));
                    vescBuffer = vescBuffer.substr(PACKET_SIZE);
                } else {
                    dumpBuffer("VESC => BLE/UART", vescBuffer);
                    pCharacteristicVescTx->setValue(vescBuffer);
                    vescBuffer.clear();
                }
                pCharacteristicVescTx->notify();
                delay(bleWait); // bluetooth stack will go into congestion, if too many packets are sent
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

    if (millis() - bleLoop > 500) {
        updateRescueApp(loopCount, loopTimeSum/loopCount, maxLoopTime);
        bleLoop = millis();
        loopTimeSum = 0;
        loopCount = 0;
    }
}

void BleServer::stop() {
    NimBLEDevice::deinit(true);
}

void BleServer::dumpBuffer(std::string header, std::string buffer) {
    if(Logger::getLogLevel() != Logger::VERBOSE) {
        return;
    }
    int length = snprintf(tmpbuf, 50, "%s : len = %d / ", header.c_str(), buffer.length());
    for (char i : buffer) {
        length += snprintf(tmpbuf+length, 1024-length, "%02x ", i);
    }
    Logger::verbose(LOG_TAG_BLESERVER, tmpbuf);
}

//NimBLECharacteristicCallbacks::onWrite
void BleServer::onWrite(BLECharacteristic *pCharacteristic) {
    snprintf(buf, bufSize, "onWrite to characteristics: %s", pCharacteristic->getUUID().toString().c_str());
    Logger::verbose(LOG_TAG_BLESERVER, buf);
    std::string rxValue = pCharacteristic->getValue();
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
            const std::string& str(rxValue);
            std::string::size_type middle = str.find('='); // Find position of '='
            std::string key;
            std::string value;
            if (middle != std::string::npos) {
                key = str.substr(0, middle);
                value = str.substr(middle + 1, str.size() - (middle + 1));
            }

            //Serial.println(String(key.c_str()) + String("=") + String(value.c_str()));

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
                AppConfiguration::getInstance()->config.minBatteryVoltage = parseDouble(value);
            } else if (key == "lowBatteryVoltage") {
                AppConfiguration::getInstance()->config.lowBatteryVoltage = parseDouble(value);
            } else if (key == "maxBatteryVoltage") {
                AppConfiguration::getInstance()->config.maxBatteryVoltage = parseDouble(value);
            } else if (key == "batteryDrift") {
                AppConfiguration::getInstance()->config.batteryDrift = parseDouble(value);
            } else if (key == "startSoundIndex") {
                AppConfiguration::getInstance()->config.startSoundIndex = parseInt(value);
                Buzzer::stopSound();
                Buzzer::playSound(RTTTL_MELODIES(strtol(value.c_str(), nullptr, 10)));
                snprintf(buf, bufSize, "Updated param \"StartSoundIndex\" to %s", value.c_str());
            } else if (key == "startLightIndex") {
                AppConfiguration::getInstance()->config.startLightIndex = parseInt(value);
            } else if (key == "batteryWarningSoundIndex") {
                AppConfiguration::getInstance()->config.batteryWarningSoundIndex = parseInt(value);
                Buzzer::stopSound();
                Buzzer::playSound(RTTTL_MELODIES(parseInt(value)));
                snprintf(buf, bufSize, "Updated param \"BatteryWarningSoundIndex\" to %s", value.c_str());
            } else if (key == "batteryAlarmSoundIndex") {
                AppConfiguration::getInstance()->config.batteryAlarmSoundIndex = parseInt(value);
                Buzzer::stopSound();
                Buzzer::playSound(RTTTL_MELODIES(parseInt(value)));
                snprintf(buf, bufSize, "Updated param \"BatteryAlarmSoundIndex\" to %s", value.c_str());
            } else if (key == "startLightDuration") {
                AppConfiguration::getInstance()->config.startLightDuration = parseInt(value);
            } else if (key == "idleLightIndex") {
                AppConfiguration::getInstance()->config.idleLightIndex = parseInt(value);
            } else if (key == "idleLightTimeout") {
                AppConfiguration::getInstance()->config.idleLightTimeout = parseInt(value);
            } else if (key == "lightFadingDuration") {
                AppConfiguration::getInstance()->config.lightFadingDuration = parseInt(value);
            } else if (key == "lightMaxBrightness") {
                AppConfiguration::getInstance()->config.lightMaxBrightness = parseInt(value);
            } else if (key == "lightColorPrimary") {
                AppConfiguration::getInstance()->config.lightColorPrimary = parseInt(value);
                snprintf(buf, bufSize, "Updated param \"lightColorPrimary\" to %s (%d)", value.c_str(),
                         AppConfiguration::getInstance()->config.lightColorPrimary);
            } else if (key == "lightColorSecondary") {
                AppConfiguration::getInstance()->config.lightColorSecondary = parseInt(value);
                snprintf(buf, bufSize, "Updated param \"lightColorSecondary\" to %s (%d", value.c_str(),
                         AppConfiguration::getInstance()->config.lightColorSecondary);
            } else if (key == "lightbarMaxBrightness") {
                AppConfiguration::getInstance()->config.lightbarMaxBrightness = parseInt(value);
            } else if (key == "lightbarTurnOffErpm") {
                AppConfiguration::getInstance()->config.lightbarTurnOffErpm = parseInt(value);
            } else if (key == "brakeLightEnabled") {
                AppConfiguration::getInstance()->config.brakeLightEnabled = parseInt(value);
            } else if (key == "brakeLightMinAmp") {
                AppConfiguration::getInstance()->config.brakeLightMinAmp = parseInt(value);
            } else if (key == "numberPixelLight") {
                AppConfiguration::getInstance()->config.numberPixelLight = parseInt(value);
            } else if (key == "numberPixelBatMon") {
                AppConfiguration::getInstance()->config.numberPixelBatMon = parseInt(value);
            } else if (key == "vescId") {
                AppConfiguration::getInstance()->config.vescId = parseInt(value);
            } else if (key == "ledType") {
                AppConfiguration::getInstance()->config.ledType = value.c_str();
            } else if (key == "ledFrequency") {
                AppConfiguration::getInstance()->config.ledFrequency = value.c_str();
            } else if (key == "logLevel") {
                AppConfiguration::getInstance()->config.logLevel = static_cast<Logger::Level>(strtol(value.c_str(), nullptr, 10));
            } else if (key == "mtuSize") {
                uint16_t mtu = strtol(value.c_str(), nullptr, 10);
                AppConfiguration::getInstance()->config.mtuSize = mtu;
                if (mtu != 0 && mtu < MTU_SIZE) {
                    NimBLEDevice::setMTU(mtu);
                    snprintf(buf, bufSize, "New MTU-size: %d", mtu);
                    Logger::warning(LOG_TAG_BLESERVER, buf);
                }
            } else if(key == "lightsSwitch") {
                AppConfiguration::getInstance()->config.lightsSwitch = ("true" == value);
            } else if(key == "update"){
                if (value == "start") { // && !updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
                    AppConfiguration::getInstance()->config.otaUpdateActive = true;
                    snprintf(buf, bufSize, "startUpdate");
                }
            }
            Logger::notice(LOG_TAG_BLESERVER, buf);
        }
    }
    delay(bleWait); // needed to give BLE stack some time
}

//NimBLECharacteristicCallbacks::onSubscribe
void BleServer::onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue) {
    snprintf(buf, bufSize, "Client ID: %d, Address: %s, Subvalue %d, Characteristics %s ",
             desc->conn_handle, NimBLEAddress(desc->peer_ota_addr).toString().c_str(), subValue,
             pCharacteristic->getUUID().toString().c_str());
    Logger::notice(LOG_TAG_BLESERVER, buf);
}

//NimBLECharacteristicCallbacks::onStatus
void BleServer::onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code) {
    if(Logger::getLogLevel() == Logger::VERBOSE) {
        snprintf(buf, bufSize, "Notification/Indication characteristics: %s, status code: %d, return code: %d", 
        pCharacteristic->getUUID().toString().c_str(), status, code);
        Logger::verbose(LOG_TAG_BLESERVER, buf);
    }
}

void BleServer::updateRescueApp(long count, long loopTime, long maxLoopTime) {
    snprintf(buf, bufSize, "%d;%d;%d", count, loopTime, maxLoopTime); 
    this->sendValue(pCharacteristicLoop, "loopTime", buf);
}

template<typename TYPE>
void BleServer::sendValue(NimBLECharacteristic *pCharacteristic, std::string key, TYPE value) {
    std::stringstream ss;
    ss << key << "=" << value;
    pCharacteristic->setValue(ss.str());
    pCharacteristic->notify();
    ss.str("");
    delay(bleWait);
}

static boolean isStringType(String a) { return true; }

static boolean isStringType(std::string a) { return true; }

static boolean isStringType(int a) { return false; }

static boolean isStringType(double a) { return false; }

static boolean isStringType(boolean a) { return false; }

int BleServer::parseInt(const std::string& strValue) {
    return strtol(strValue.c_str(), nullptr, 10);
}

double BleServer::parseDouble(const std::string& strValue) {
    return strtod(strValue.c_str(), nullptr);
}

struct BleServer::sendConfigValue {
    NimBLECharacteristic *pCharacteristic;
    std::stringstream ss;
    char localbuf[128];

    explicit sendConfigValue(NimBLECharacteristic *pCharacteristic) {
        this->pCharacteristic = pCharacteristic;
    }

    template<typename T>
    void operator()(const char *name, const T &value) {
        if (isStringType(value)) {
            ss << name << "=" << static_cast<String>(value).c_str();
        } else {
            ss << name << "=" << value;
        }
        snprintf(localbuf, 128, "Sending %s", ss.str().c_str());
        Logger::notice(LOG_TAG_BLESERVER, localbuf);

        pCharacteristic->setValue(ss.str());
        pCharacteristic->indicate();
        ss.str("");
        delay(5*bleWait);
    }
};

void BleServer::sendConfig() {
    visit_struct::for_each(AppConfiguration::getInstance()->config, sendConfigValue(pCharacteristicConf));
}