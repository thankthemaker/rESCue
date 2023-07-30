#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#include <Arduino.h>
#include "config.h"
#include "CanBus.h"
#include "AppConfiguration.h"
#include "Buzzer.h"
#include <NimBLEDevice.h>
#include "base64.h"

#define LOG_TAG_BLESERVER "BleServer"

#define VESC_SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define VESC_CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define VESC_CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define RESCUE_SERVICE_UUID                   "99EB1511-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_ID         "99EB1512-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_CONF       "99EB1513-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_FW         "99EB1514-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_HW_VERSION "99EB1515-A9E9-4024-B0A4-3DC4B4FABFB0"
#define RESCUE_CHARACTERISTIC_UUID_LOOP       "99EB1516-A9E9-4024-B0A4-3DC4B4FABFB0"

class BleServer :
  public NimBLEServerCallbacks,
  public BLECharacteristicCallbacks  {
    public:
      BleServer();
      void init(Stream *vesc, CanBus *canbus);
      void loop(VescData *vescData, unsigned long loopTime, unsigned long maxLoopTime);
      void stop();

      // NimBLEServerCallbacks
      void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
      void onDisconnect(NimBLEServer* pServer) override;
      void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override;

      // NimBLECharacteristicCallbacks
      void onWrite(NimBLECharacteristic* pCharacteristic) override;
      void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) override;
      void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) override;
      static void sendConfig();
      template<typename TYPE>
      void sendValue(NimBLECharacteristic *pCharacteristic, std::string key, TYPE value);
      void updateRescueApp(long loopTime, long maxLoopTime);

    private:
      const static int bufSize = 256;
      char buf[bufSize];
      CanBus *canbus{};
      struct sendConfigValue;
      static void dumpBuffer(std::string header, std::string buffer);
      static int parseInt(const std::string& strValue);
      static double parseDouble(const std::string& strValue);
};

#endif