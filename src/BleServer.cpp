#include "BleServer.h"
#include "BlynkPins.h"
#include <Logger.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"

NimBLEServer* pServer = NULL;
NimBLEService* pServiceBlynk = NULL;
NimBLEService* pServiceVesc = NULL;
NimBLECharacteristic* pCharacteristicBlynkTx = NULL;
NimBLECharacteristic* pCharacteristicBlynkRx = NULL;
NimBLECharacteristic* pCharacteristicVescTx = NULL;
NimBLECharacteristic* pCharacteristicVescRx = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
BlynkFifo<uint8_t, BLYNK_MAX_READBYTES*2> mBuffRX;
Stream *vescSerial;
std::string bufferString;
int lastLoop = 0;

BLYNK_WRITE_DEFAULT() {
  int pin = request.pin;
  char buf[128];
  switch (pin) {
    case VPIN_APP_LIGHT_INDEX:
      snprintf(buf, 128, "Updated param \"LightIndex\" to %d", param.asInt());
      break;
    case VPIN_APP_SOUND_INDEX:
      snprintf(buf, 128, "Updated param \"SoundIndex\" to %d", param.asInt());
      break;
    case VPIN_APP_MIN_BAT_VOLTAGE:
      snprintf(buf, 128, "Updated param \"MinBatVoltage\" to %f", param.asDouble());
      break;
    case VPIN_APP_MAX_BAT_VOLTAGE:
      snprintf(buf, 128, "Updated param \"MaxBatVoltage\" to %f", param.asDouble());
      break;
    case VPIN_APP_NOTIFICATION:
      snprintf(buf, 128, "Updated param \"NotificationEnabled\" to %d", param.asInt());
      break;
  }
  Logger::notice(LOG_TAG_BLESERVER, buf);
}

class BlynkEsp32_BLE : public BlynkProtocol<BleServer> {
    typedef BlynkProtocol<BleServer> Base;
    public:
      BlynkEsp32_BLE(BleServer& transp) : Base(transp) {}

      void begin(const char* auth) {
        Logger::verbose(LOG_TAG_BLESERVER, "BlynkEsp32_BLE::begin");
        Base::begin(auth);
        state = DISCONNECTED;
        conn.begin();
      }

      void setDeviceName(const char* name) {}
};
    
static BleServer _blynkTransportBLE;
BlynkEsp32_BLE Blynk(_blynkTransportBLE);

BleServer::BleServer(): 
  mConn (false), 
  mName (BT_NAME) {}

// NimBLEServerCallbacks::onConnect
inline
void BleServer::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
  char buf[128];
  snprintf(buf, 128, "Client connected: %s", NimBLEAddress(desc->peer_ota_addr).toString().c_str());
  Logger::notice(LOG_TAG_BLESERVER, buf);
  Logger::notice(LOG_TAG_BLESERVER, "Multi-connect support: start advertising");
  deviceConnected = true;
  BLYNK_LOG1(BLYNK_F("BLE connect"));
  connect();
  Blynk.startSession();
  NimBLEDevice::startAdvertising();
};

// NimBLEServerCallbacks::onDisconnect
inline
void BleServer::onDisconnect(NimBLEServer* pServer) {
  Logger::notice(LOG_TAG_BLESERVER, "Client disconnected - start advertising");
  deviceConnected = false;
  BLYNK_LOG1(BLYNK_F("BLE disconnect"));
  Blynk.disconnect();
  disconnect();
  NimBLEDevice::startAdvertising();
}

void BleServer::init(Stream *vesc) {
  vescSerial = vesc;
   
  // Create the BLE Device
  NimBLEDevice::init(BT_NAME);

  // Create the BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(this);
  auto pSecurity = new NimBLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  // Create the BLE Service
  pServiceBlynk = pServer->createService(BLYNK_SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristicBlynkTx = pServiceBlynk->createCharacteristic(
                      BLYNK_CHARACTERISTIC_UUID_TX,  
                      NIMBLE_PROPERTY::NOTIFY
                    );
  //pCharacteristicBlynkTx->setValue("BLYNK TX");
  pCharacteristicBlynkTx->setCallbacks(this);

  // Create a BLE Characteristic
  pCharacteristicBlynkRx = pServiceBlynk->createCharacteristic(
                      BLYNK_CHARACTERISTIC_UUID_RX,  
                      NIMBLE_PROPERTY::WRITE
                    );
  //pCharacteristicBlynkRx->setValue("BLYNK RX");
  pCharacteristicBlynkRx->setCallbacks(this);

  // Start the service
  pServiceBlynk->start();

  // Create the VESC BLE Service
  pServiceVesc = pServer->createService(VESC_SERVICE_UUID);

  // Create a BLE Characteristic for VESC TX
  pCharacteristicVescTx = pServiceVesc->createCharacteristic(
                      VESC_CHARACTERISTIC_UUID_TX,  
                      NIMBLE_PROPERTY::NOTIFY   |
                      NIMBLE_PROPERTY::READ
                    );
  //pCharacteristicVescTx->setValue("VESC TX");
  pCharacteristicVescTx->setCallbacks(this);

  // Create a BLE Characteristic for VESC RX
  pCharacteristicVescRx = pServiceVesc->createCharacteristic(
                      VESC_CHARACTERISTIC_UUID_RX,  
                      NIMBLE_PROPERTY::READ   |
                      NIMBLE_PROPERTY::WRITE
                    );
  //pCharacteristicVescRx->setValue("VESC RX");
  pCharacteristicVescRx->setCallbacks(this);

  // Start the VESC service
  pServiceVesc->start();

  // Start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(VESC_SERVICE_UUID);
  pAdvertising->addServiceUUID(BLYNK_SERVICE_UUID);
  pAdvertising->setAppearance(0x00);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  Logger::verbose(LOG_TAG_BLESERVER, "starting Blynk");
  Blynk.setDeviceName(BT_NAME);
  Blynk.begin(BLYNK_AUTH_TOKEN);

  NimBLEDevice::startAdvertising();
  Logger::notice(LOG_TAG_BLESERVER, "waiting a client connection to notify...");
}

#ifdef CANBUS
void BleServer::loop(CanBus::VescData *vescData) {
#else
void BleServer::loop() {
#endif
  if(vescSerial->available()) {
    int oneByte;
    while(vescSerial->available()) {
      oneByte = vescSerial->read();
      bufferString.push_back(oneByte);
    }
    if(Logger::getLogLevel() == Logger::VERBOSE) {
      Logger::verbose(LOG_TAG_BLESERVER, "BLE from VESC: ");
      Logger::verbose(LOG_TAG_BLESERVER, bufferString.c_str());
    }

    if (deviceConnected) {
//      while(bufferString.length() > 600) {
        pCharacteristicVescTx->setValue(bufferString.substr(0, 600));
        pCharacteristicVescTx->notify();
//        delay(10);
//        bufferString = bufferString.substr(600);
//      }
      bufferString.clear();
		  delay(10); // bluetooth stack will go into congestion, if too many packets are sent
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

  Blynk.run();
    
#ifdef CANBUS
  if(millis() - lastLoop > 1000) {
    updateBlynk(vescData);
    lastLoop = millis();
  }
#endif
}

// IP redirect not available
void BleServer::begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p) {}

void BleServer::begin() {
  Logger::verbose(LOG_TAG_BLESERVER, "begin");
}

bool BleServer::connect() {
  Logger::verbose(LOG_TAG_BLESERVER, "connect");
  mBuffRX.clear();
  return mConn = true;
}

void BleServer::disconnect() {
  Logger::verbose(LOG_TAG_BLESERVER, "disconnect");
  mConn = false;
}

bool BleServer::connected() { 
  //Logger::verbose(LOG_TAG_BLESERVER, "BleServer::connected: " + mConn);
  return mConn;
}

size_t BleServer::read(void* buf, size_t len) {
  Logger::verbose(LOG_TAG_BLESERVER, "read");
  millis_time_t start = BlynkMillis();
  while (BlynkMillis() - start < BLYNK_TIMEOUT_MS) {
    if (available() < len) {
      delay(1);
    } else {
      break;
    }
  }
  size_t res = mBuffRX.get((uint8_t*)buf, len);
  return res;
}

size_t BleServer::write(const void* buf, size_t len) {
  char buffer[128];
  snprintf(buffer, 128, "write: %d", (uint8_t*)buf);
  Logger::verbose(LOG_TAG_BLESERVER, buffer);
  pCharacteristicBlynkTx->setValue((uint8_t*)buf, len);
  pCharacteristicBlynkTx->notify();
  return len;
}

size_t BleServer::available() {
  size_t rxSize = mBuffRX.size();
  //Logger::verbose(LOG_TAG_BLESERVER, "BleServer::available: " + rxSize);
  return rxSize;
}

//NimBLECharacteristicCallbacks::onWrite
void BleServer::onWrite(BLECharacteristic *pCharacteristic) {
  char buffer[128];
  snprintf(buffer, 128, "onWrite to characteristics: %s", pCharacteristic->getUUID().toString().c_str());
  Logger::notice(LOG_TAG_BLESERVER, buffer);
  std::string rxValue = pCharacteristic->getValue();
  size_t len = rxValue.length();
  snprintf(buffer, 128, "onWrite - value: %s", rxValue.data());
  Logger::notice(LOG_TAG_BLESERVER, buffer);
  if (rxValue.length() > 0) {
    if(pCharacteristic->getUUID().equals(pCharacteristicVescRx->getUUID())) {
      for (int i = 0; i < rxValue.length(); i++) {
        vescSerial->write(rxValue[i]);
      }
    } else if (pCharacteristic->getUUID().equals(pCharacteristicBlynkRx->getUUID())) {
      uint8_t* data = (uint8_t*)rxValue.data();
      BLYNK_DBG_DUMP(">> ", data, len);
      mBuffRX.put(data, len);
    }
  }
}

//NimBLECharacteristicCallbacks::onSubscribe
void BleServer::onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
  char buf[256];
  snprintf(buf, 256, "Client ID: %d, Address: %s, Subvalue %d, Characteristics %s ",
    desc->conn_handle, NimBLEAddress(desc->peer_ota_addr).toString().c_str(), subValue, pCharacteristic->getUUID().toString().c_str());
  Logger::notice(LOG_TAG_BLESERVER, buf);
}

//NimBLECharacteristicCallbacks::onSubscribe
void BleServer::onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
  char buf[256];
  snprintf(buf, 256, "Notification/Indication status code: %d, return code: %d", status, code);
  Logger::verbose(LOG_TAG_BLESERVER, buf);
}

#ifdef CANBUS
void BleServer::updateBlynk(CanBus::VescData *vescData) {
  Blynk.virtualWrite(VPIN_VESC_INPUT_VOLTAGE, vescData->inputVoltage);
  Blynk.virtualWrite(VPIN_VESC_ERPM, vescData->erpm);
  Blynk.virtualWrite(VPIN_VESC_DUTY_CYCLE, vescData->dutyCycle);
  Blynk.setProperty(VPIN_VESC_DUTY_CYCLE, "color", 
    vescData->dutyCycle > 0 ? BLINK_COLOR_GREEN : BLINK_COLOR_RED);
  Blynk.virtualWrite(VPIN_VESC_MOSFET_TEMP, vescData->mosfetTemp);
  Blynk.virtualWrite(VPIN_VESC_TACHOMETER, vescData->tachometer);   
  Blynk.virtualWrite(VPIN_VESC_MOTOR_TEMP, vescData->motorTemp);
  Blynk.virtualWrite(VPIN_VESC_AMP_HOURS, vescData->ampHours);
  Blynk.virtualWrite(VPIN_VESC_AMP_HOURS_CHARGED, vescData->ampHoursCharged);
  Blynk.virtualWrite(VPIN_VESC_WATT_HOURS, vescData->wattHours);
  Blynk.virtualWrite(VPIN_VESC_WATT_HOURS_CHARGED, vescData->wattHoursCharged);
  Blynk.virtualWrite(VPIN_VESC_CURRENT, vescData->current);
}
#endif