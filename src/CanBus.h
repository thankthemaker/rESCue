#ifndef __CANBUS_H__
#define __CANBUS_H__

#include "Arduino.h"
#include "config.h"

#include "AppConfiguration.h"
#include <LoopbackStream.h>
#include "VescCanConstants.h"
#include "BleCanProxy.h"
#include "CanDevice.h"
#include "VescData.h"
#include <vector>
#include "buffer.h"
#define B10000001 129
#define B11000011 195

static const char* LOG_TAG_CANBUS = "CanBus";

class CanBus {
    public:
      CanBus(VescData *vescData);
      VescData *vescData;
      LoopbackStream *stream;
      BleCanProxy *proxy;
      void init();
      void loop();
      void dumpVescValues();
      boolean isInitialized();
      int getInterval();
      boolean bmsVTOT(float,float);
      boolean bmsVCell(const uint16_t*,int);
      boolean bmsTemps(const int8_t*,int);
      boolean bmsBal(boolean);
      boolean bmsI(float);
      boolean bmsAHWH(float,float);
      boolean bmsAHWHDischargeTotal(float,float);
      boolean bmsAHWHChargeTotal(float,float);
      boolean bmsSOCSOHTempStat(float,float,float,float,float,boolean,boolean,boolean,boolean);
      boolean bmsState(bms_op_state op_state, bms_fault_state fault_state);
    private:
      const static int bufSize = 128;
      char buf[bufSize];
      CanDevice *candevice;
      boolean requestFirmwareVersion();
      boolean requestRealtimeData();
      boolean requestBalanceData();
      boolean requestFloatPackageData();
      void ping();
      static void clearFrame(twai_message_t rx_frame);
      static void printFrame(twai_message_t rx_frame, int frameCount);
      void processFrame(twai_message_t rx_frame, int frameCount);
      float readFloatValueFromBuffer(int startbyte, boolean isProxyRequest);
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