#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#define BLYNK_USE_DIRECT_CONNECT
#ifndef BLYNK_INFO_CONNECTION
 #define BLYNK_INFO_CONNECTION "Esp32_NimBLE"
#endif
#define BLYNK_SEND_ATOMIC
#define BLYNK_SEND_CHUNK 20
//#define BLYNK_SEND_THROTTLE 20

#include <Arduino.h>
#include "config.h"
#include "CanBus.h"
#include <NimBLEDevice.h>
#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <utility/BlynkFifo.h>

#define LOG_TAG_BLESERVER "BleServer"

#define VESC_SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define VESC_CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define VESC_CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLYNK_SERVICE_UUID           "713D0000-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_RX "713D0003-503E-4C75-BA94-3148F18D941E"
#define BLYNK_CHARACTERISTIC_UUID_TX "713D0002-503E-4C75-BA94-3148F18D941E"

extern BlynkFifo<uint8_t, BLYNK_MAX_READBYTES*2> mBuffRX;

class BleServer : 
  public NimBLEServerCallbacks, 
  public BLECharacteristicCallbacks  {
    public:
      BleServer();
      void init(Stream *vesc);
#ifdef CANBUS_ENABLED
      void loop(CanBus::VescData *vescData);
#else
      void loop();
#endif

      // Blynk stuff
      void begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p);
      void begin();
      bool connect();
      void disconnect();
      bool connected();
      size_t read(void* buf, size_t len);
      size_t write(const void* buf, size_t len);
      size_t available();

      // NimBLEServerCallbacks
      void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
      void onDisconnect(NimBLEServer* pServer);

      // NimBLECharacteristicCallbacks
      void onWrite(NimBLECharacteristic* pCharacteristic);
      void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue);
      void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code);

#ifdef CANBUS_ENABLED
      void updateBlynk(CanBus::VescData *vescData);
#endif

    private:
      bool mConn;
      const char* mName;
};

#endif