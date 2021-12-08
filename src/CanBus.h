#ifndef __CANBUS_H__
#define __CANBUS_H__


#include "Arduino.h"
#include "config.h"
#include "AppConfiguration.h"
#include <LoopbackStream.h>

#ifdef CANBUS_ENABLED

#include <ESP32CAN.h>
#include <CAN_config.h>

#define BUFFER_SIZE 65535

#define B10000001 129
#define B11000011 195

#define LOG_TAG_CANBUS "CanBus"

typedef enum {
  CAN_PACKET_SET_DUTY = 0,
  CAN_PACKET_SET_CURRENT,
  CAN_PACKET_SET_CURRENT_BRAKE,
  CAN_PACKET_SET_RPM,
  CAN_PACKET_SET_POS,
  CAN_PACKET_FILL_RX_BUFFER,
  CAN_PACKET_FILL_RX_BUFFER_LONG,
  CAN_PACKET_PROCESS_RX_BUFFER,
  CAN_PACKET_PROCESS_SHORT_BUFFER,
  CAN_PACKET_STATUS,
  CAN_PACKET_SET_CURRENT_REL,
  CAN_PACKET_SET_CURRENT_BRAKE_REL,
  CAN_PACKET_SET_CURRENT_HANDBRAKE,
  CAN_PACKET_SET_CURRENT_HANDBRAKE_REL,
  CAN_PACKET_STATUS_2,
  CAN_PACKET_STATUS_3,
  CAN_PACKET_STATUS_4,
  CAN_PACKET_PING,
  CAN_PACKET_PONG,
  CAN_PACKET_DETECT_APPLY_ALL_FOC,
  CAN_PACKET_DETECT_APPLY_ALL_FOC_RES,
  CAN_PACKET_CONF_CURRENT_LIMITS,
  CAN_PACKET_CONF_STORE_CURRENT_LIMITS,
  CAN_PACKET_CONF_CURRENT_LIMITS_IN,
  CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN,
  CAN_PACKET_CONF_FOC_ERPMS,
  CAN_PACKET_CONF_STORE_FOC_ERPMS,
  CAN_PACKET_STATUS_5,
  CAN_PACKET_POLL_TS5700N8501_STATUS,
  CAN_PACKET_CONF_BATTERY_CUT,
  CAN_PACKET_CONF_STORE_BATTERY_CUT,
  CAN_PACKET_SHUTDOWN,
  CAN_PACKET_IO_BOARD_ADC_1_TO_4,
  CAN_PACKET_IO_BOARD_ADC_5_TO_8,
  CAN_PACKET_IO_BOARD_ADC_9_TO_12,
  CAN_PACKET_IO_BOARD_DIGITAL_IN,
  CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL,
  CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM,
  CAN_PACKET_BMS_V_TOT,
  CAN_PACKET_BMS_I,
  CAN_PACKET_BMS_AH_WH,
  CAN_PACKET_BMS_V_CELL,
  CAN_PACKET_BMS_BAL,
  CAN_PACKET_BMS_TEMPS,
  CAN_PACKET_BMS_HUM,
  CAN_PACKET_BMS_SOC_SOH_TEMP_STAT
} CAN_PACKET_ID;

extern CAN_device_t CAN_cfg;

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
      void init();
      void loop();
      void dumpVescValues();
      void proxyIn(std::string in);
      void proxyOut(uint8_t *data, int size, uint8_t crc1, uint8_t crc2);
    private:
      void requestFirmwareVersion();
      void requestRealtimeData();
      void requestBalanceData();
      void ping();
      void printFrame(CAN_frame_t rx_frame, int frameCount);
      void processFrame(CAN_frame_t rx_frame, int frameCount);
      void sendCanFrame(const CAN_frame_t* p_frame);
      int32_t readInt32Value(CAN_frame_t rx_frame, int startbyte);
      int16_t readInt16Value(CAN_frame_t rx_frame, int startbyte);
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
      SemaphoreHandle_t mutex_v = xSemaphoreCreateMutex();
      uint16_t length = 0;
      uint8_t command = 0;
      boolean longPacket = false;
      std::string longPackBuffer;
      int initRetryCounter = 5;
      int lastDump = 0;
      int lastRetry = 0;
      int lastStatus = 0;
      int lastRealtimeData = 0;
      int lastBalanceData = 0;
      std::vector<uint8_t> buffer = {};
      std::vector<uint8_t> proxybuffer = {};
};

#endif //CANBUS_ENABLED
#endif //__CANBUS_H__