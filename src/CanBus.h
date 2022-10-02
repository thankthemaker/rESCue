#ifndef __CANBUS_H__
#define __CANBUS_H__


#include "Arduino.h"
#include "config.h"


#ifdef CANBUS_ENABLED

#include <ESP32CAN.h>
#include "AppConfiguration.h"
#include <LoopbackStream.h>
#include <Logger.h>
#include "CAN.h"
#include "CAN_config.h"
#include "VescCanConstants.h"
#include "BleCanProxy.h"
#include "CanDevice.h"
#include <vector>

#define B10000001 129
#define B11000011 195

#define LOG_TAG_CANBUS "CanBus"

class CanBus {
    public:
      struct VescData {
        uint8_t majorVersion;
        uint8_t minorVersion;
        std::string name;
        std::string uuid;

        double dutyCycle = 0.0;
        double erpm = 0.0;
        double current = 0.0;
        double ampHours = 0.0;
        double ampHoursCharged= 0.0;
        double wattHours = 0.0;
        double wattHoursCharged = 0.0;
        double mosfetTemp = 0.0;
        double motorTemp = 0.0;
        double totalCurrentIn = 0.0;
        double pidPosition = 0.0;
        double inputVoltage = 0.0;
        double tachometer = 0.0;
        double tachometerAbsolut = 0.0;

        double pidOutput = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
        uint32_t loopTime = 0;
        double motorCurrent = 0.0;
        double motorPosition = 0.0;
        uint16_t balanceState = 0;
        uint16_t switchState = 0;
        double adc1 = 0.0;
        double adc2 = 0.0;
        uint8_t fault = 0;
    } vescData;

      CanBus();
      LoopbackStream *stream;
      BleCanProxy *proxy;
      void init();
      void loop();
      void dumpVescValues();
    private:
      CanDevice *candevice;
      void requestFirmwareVersion();
      void requestRealtimeData();
      void requestBalanceData();
      void ping();
      static void printFrame(CAN_frame_t rx_frame, int frameCount);
      void processFrame(CAN_frame_t rx_frame, int frameCount);
      void clearFrame(CAN_frame_t rx_frame);
      static int32_t readInt32Value(CAN_frame_t rx_frame, int startbyte);
      static int16_t readInt16Value(CAN_frame_t rx_frame, int startbyte);
      int32_t readInt32ValueFromBuffer(int startbyte, boolean isProxyRequest);
      int16_t readInt16ValueFromBuffer(int startbyte, boolean isProxyRequest);
      int8_t readInt8ValueFromBuffer(int startbyte, boolean isProxyRequest);
      std::string readStringValueFromBuffer(int startbyte, int length, boolean isProxyRequest);
      uint8_t vesc_id;
      uint8_t esp_can_id;
      uint8_t ble_proxy_can_id;
      int32_t RECV_STATUS_1;
      int32_t RECV_STATUS_2;
      int32_t RECV_STATUS_3;
      int32_t RECV_STATUS_4;
      int32_t RECV_STATUS_5;
      uint32_t RECV_FILL_RX_BUFFER;
      uint32_t RECV_PROCESS_RX_BUFFER;
      uint32_t RECV_PROCESS_SHORT_BUFFER_PROXY;
      uint32_t RECV_FILL_RX_BUFFER_PROXY;
      uint32_t RECV_FILL_RX_BUFFER_LONG_PROXY;
      uint32_t RECV_PROCESS_RX_BUFFER_PROXY;
      boolean initialized = false;
      int interval = 500;
      int initRetryCounter = 5;
      unsigned long lastDump = 0;
      unsigned long lastRetry = 0;
      unsigned long lastRealtimeData = 0;
      unsigned long lastBalanceData = 0;
      std::vector<uint8_t> buffer = {};
      std::vector<uint8_t> proxybuffer = {};
};

#endif //CANBUS_ENABLED
#endif //__CANBUS_H__