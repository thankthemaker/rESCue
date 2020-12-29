#include "btbridge.h"
#include <HardwareSerial.h>

HardwareSerial vescSerial(2);
BluetoothSerial bleSerial;

BluetoothBridge::BluetoothBridge() {}

void BluetoothBridge::init() {
    bleSerial.begin(BT_NAME);
    vescSerial.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);         
}

void BluetoothBridge::loop() {
  if (vescSerial.available()) {
    bleSerial.write(Serial.read());
  }
  if (bleSerial.available()) {
    vescSerial.write(bleSerial.read());
  }
}
