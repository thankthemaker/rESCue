#include "BleBridge.h"

#ifdef ESP32

Stream *vescSerial;
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t incomingByte = -1;

BleBridge::BleBridge() {}

void initBle(){

  // Create the BLE Device
  BLEDevice::init(BT_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new BleCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
#if DEBUG > 0
  Serial.println("Waiting for a BLE client connection...");
#endif
}

void BleBridge::init(Stream* vesc) {
#if DEBUG > 0
  Serial.println("Initializing BleBridge");
#endif
  initBle();
  vescSerial = vesc;  
}

void BleBridge::loop() {
  if(vescSerial->available()) {
    incomingByte = vescSerial->read();
    if (deviceConnected) {
      pTxCharacteristic->setValue(&incomingByte, 1);
      pTxCharacteristic->notify();
		  delay(3); // bluetooth stack will go into congestion, if too many packets are sent
	  }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
#if DEBUG > 0
    Serial.println("start advertising");
#endif
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

#endif // BLE_ENABLED
