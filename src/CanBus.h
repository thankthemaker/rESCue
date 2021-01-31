#ifndef __CANBUS_H__
#define __CANBUS_H__


#include "Arduino.h"
#include "config.h"

#ifdef CANBUS_ENABLED

#include <ESP32CAN.h>
#include <CAN_config.h>

#define B10000001 129
#define B11000011 195

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
        double dutyCycle;
        double erpm;
        double current;
        double ampHours;
        double ampHoursCharged;
        double wattHours;
        double wattHoursCharged;
        double mosfetTemp;
        double motorTemp;
        double inputVoltage;
        double tachometer;
      } vescData;

      CanBus();
      void init();
      void loop();
      void dumpVescValues();
    private:

      void requestRealtimeData();
      void requestBalanceData();
      void ping();
      void printFrame(CAN_frame_t rx_frame);
      void processFrame(CAN_frame_t rx_frame);
      int32_t readInt32Value(CAN_frame_t rx_frame, int startbyte);
      int16_t readInt16Value(CAN_frame_t rx_frame, int startbyte);
};

#endif //CANBUS_ENABLED
#endif //__CANBUS_H__