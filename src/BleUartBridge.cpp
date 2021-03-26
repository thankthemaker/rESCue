#include "BleUartBridge.h"

#ifdef ESP32

Stream *vescSerial;
std::string bufferString;

BleUartBridge::BleUartBridge(BLEServer *pServer) {
  this->pServer = pServer;
}

void BleUartBridge::init(Stream* vesc) {
#if DEBUG > 0
  Serial.println("Initializing BleBridge");
#endif
  vescSerial = vesc;  
}

void BleUartBridge::loop() {
  if(vescSerial->available()) {
    int oneByte;
#if DEBUG > 2
   Serial.print("\nBLE from VESC: ");
#endif
   while(vescSerial->available()) {
      oneByte = vescSerial->read();
      bufferString.push_back(oneByte);
#if DEBUG > 2
      Serial.print(oneByte, HEX);
#endif
    }

    if (deviceConnected) {
//      while(bufferString.length() > 600) {
        pTxCharacteristicUart->setValue(bufferString.substr(0, 600));
        pTxCharacteristicUart->notify();
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
