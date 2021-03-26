#ifndef __BLE_BLYNK_CALLBACKS_H__
#define __BLE_BLYNK_CALLBACKS_H__

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <utility/BlynkFifo.h>

extern bool mConn;
extern BlynkFifo<uint8_t, BLYNK_MAX_READBYTES*2> mBuffRX;

class BleBlynkCallbacks : public BLECharacteristicCallbacks {
      void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        uint8_t* data = (uint8_t*)rxValue.data();
        size_t len = rxValue.length();

        BLYNK_DBG_DUMP(">> ", data, len);
        mBuffRX.put(data, len);
      }
    }
};
#endif