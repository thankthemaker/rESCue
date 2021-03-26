#include "BleServer.h"

bool deviceConnected;

BleServer::BleServer() {
  this->bleUartBridge = new BleUartBridge(this->pServer);
  this->bleBlynkApp = new BleBlynkApp(this->pServer);
}

void BleServer::init() {

  // Create the BLE Device
  BLEDevice::init(BT_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  // Create the BLE UART Service
  BLEService *pServiceUart = pServer->createService(VESC_SERVICE_UUID);

  // Create a BLE Characteristic for RX and TX
  bleUartBridge->pTxCharacteristicUart = pServiceUart->createCharacteristic(
										VESC_CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY | 
                    BLECharacteristic::PROPERTY_READ );
                      
  bleUartBridge->pTxCharacteristicUart->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristicUart = pServiceUart->createCharacteristic(
											 VESC_CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE | 
                      BLECharacteristic::PROPERTY_READ );

  pRxCharacteristicUart->setCallbacks(new BleUartCallbacks());

  // Create the BLE Blynk Service
  BLEService *pServiceBlynk = pServer->createService(BLYNK_SERVICE_UUID);

  // Create a BLE Characteristic for Blynk
  bleBlynkApp->pTxCharacteristicBlynk = pServiceBlynk->createCharacteristic(
                      BLYNK_CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY);

  bleBlynkApp->pTxCharacteristicBlynk->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristicBlynk = pServiceBlynk->createCharacteristic(
                                        BLYNK_CHARACTERISTIC_UUID_RX,
                                        BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristicBlynk->setCallbacks(new BleBlynkCallbacks());

  // Start the UARTservice
  pServiceUart->start();
  // Start the service
  pServiceBlynk->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(VESC_SERVICE_UUID);
  pServer->getAdvertising()->addServiceUUID(BLYNK_SERVICE_UUID);
  pServer->getAdvertising()->start();
#if DEBUG > 0
  Serial.println("Waiting for a BLE client connection...");
#endif
}