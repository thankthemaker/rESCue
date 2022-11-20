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

class CanDevice {
  private:
    SemaphoreHandle_t mutex_v = xSemaphoreCreateMutex();
  public:
    void init();
    void sendCanFrame(const twai_message_t *p_frame);
};
#endif //RESCUE_CANDEVICE_H
