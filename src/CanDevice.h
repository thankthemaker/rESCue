#ifndef RESCUE_CANDEVICE_H
#define RESCUE_CANDEVICE_H

#include "Arduino.h"
#include "config.h"
#include <Logger.h>
#include "driver/twai.h"

#define LOG_TAG_CANDEVICE "CanDevice"

#ifndef CAN_TX_PIN
#define CAN_TX_PIN 26
#endif //CAN_TX_PIN

#ifndef CAN_RX_PIN
#define CAN_RX_PIN 27
#endif //CAN_RX_PIN

//Macros to fix actually being able to map the CAN GPIO pins in platformio.ini instead of them being hard coded in init() for TWAI_GENERAL_CONFIG_DEFAULT();
#ifndef ESP32S3
  #define GPIO_NUM_HELPER(x) GPIO_NUM##x
  #define GPIO_NUM(x) GPIO_NUM_HELPER(x)
  #define GPIO_CAN_TX_PIN GPIO_NUM(CAN_TX_PIN)
  #define GPIO_CAN_RX_PIN GPIO_NUM(CAN_RX_PIN)
#else
  #define GPIO_NUM_HELPER(x) GPIO_NUM_##x
  #define GPIO_NUM_(x) GPIO_NUM_HELPER(x)
  #define GPIO_CAN_TX_PIN GPIO_NUM_(CAN_TX_PIN)
  #define GPIO_CAN_RX_PIN GPIO_NUM_(CAN_RX_PIN)
#endif

class CanDevice {
  private:
    const static int bufSize = 128;
    char buf[bufSize];
    SemaphoreHandle_t mutex_v = xSemaphoreCreateMutex();
  public:
    boolean init();
    boolean sendCanFrame(const twai_message_t *p_frame);
};
#endif //RESCUE_CANDEVICE_H
