#ifndef __CANBUS_H__
#define __CANBUS_H__

#include "Arduino.h"
#include "config.h"

#include "AppConfiguration.h"
#include <LoopbackStream.h>
#include <Logger.h>
#include "VescCanConstants.h"
#include "BleCanProxy.h"
#include "CanDevice.h"
#include "VescData.h"
#include <vector>

#define B10000001 129
#define B11000011 195

#define LOG_TAG_CANBUS "CanBus"

class CanBus {
    public:
      CanBus(VescData *vescData);
      VescData *vescData;
      LoopbackStream *stream;
      BleCanProxy *proxy;
      void init();
      void loop();
      void dumpVescValues();
    private:
      const static int bufSize = 128;
      char buf[bufSize];
      CanDevice *candevice;
      boolean requestFirmwareVersion();
      boolean requestRealtimeData();
      boolean requestBalanceData();
      void ping();
      static void clearFrame(twai_message_t rx_frame);
      static void printFrame(twai_message_t rx_frame, int frameCount);
      void processFrame(twai_message_t rx_frame, int frameCount);
      static int32_t readInt32Value(twai_message_t rx_frame, int startbyte);
      static int16_t readInt16Value(twai_message_t rx_frame, int startbyte);
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

#endif //__CANBUS_H__