#ifndef BLUETOOTH_BRIDGE_H
#define BLUETOOTH_BRIDGE_H

#include "config.h"
#include "BluetoothSerial.h"
#include "esp_bt.h"

class BluetoothBridge {
    public:
        BluetoothBridge();
        void init();
        void loop();
};

#endif