#include "BleBlynkApp.h"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "l7b1ongYuGiRcnDHufxwnazMH6hAn3Xl";

bool mConn;
BlynkFifo<uint8_t, BLYNK_MAX_READBYTES*2> mBuffRX;

BleBlynkApp::BleBlynkApp(BLEServer *pServer) {
  this->pServer = pServer;
}

void BleBlynkApp::onConnect() {
  BLYNK_LOG1(BLYNK_F("BLE connect"));
  connect();
  Blynk.startSession();  
}

void BleBlynkApp::onDisconnect() {
  BLYNK_LOG1(BLYNK_F("BLE disconnect"));
  Blynk.disconnect();
  disconnect(); 
}

void BleBlynkApp::init() {
  Serial.println("Waiting for connections...");
  Blynk.setDeviceName("Blynk");
  Blynk.begin(auth);
}

void BleBlynkApp::loop(){
  Blynk.run();
}

void BleBlynkApp::setDeviceName(const char* name) {}

// IP redirect not available
void BleBlynkApp::begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p) {}

void BleBlynkApp::begin() {}

bool BleBlynkApp::connect() {
  mBuffRX.clear();
  return mConn = true;
}

void BleBlynkApp::disconnect() {
  mConn = false;
}

bool BleBlynkApp::connected() {
  return mConn;
}

size_t BleBlynkApp::read(void* buf, size_t len) {
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

size_t BleBlynkApp::write(const void* buf, size_t len) {
  pTxCharacteristicBlynk->setValue((uint8_t*)buf, len);
  pTxCharacteristicBlynk->notify();
  return len;
}

size_t BleBlynkApp::available() {
  size_t rxSize = mBuffRX.size();
  return rxSize;
}