#include "CanDevice.h"

CAN_device_t CAN_cfg;

void CanDevice::init() {
    CAN_cfg.speed = CAN_SPEED_500KBPS;
    CAN_cfg.tx_pin_id = GPIO_NUM_26;
    CAN_cfg.rx_pin_id = GPIO_NUM_27;
    CAN_cfg.rx_queue = xQueueCreate(1000, sizeof(CAN_frame_t));

    /*
      CAN_filter_t p_filter;
      p_filter.FM = Single_Mode;

      p_filter.ACR0 = 0x00;
      p_filter.ACR1 = 0x00;
      p_filter.ACR2 = 0x10;
      p_filter.ACR3 = 0x19;

      p_filter.AMR0 = 0x1F;
      p_filter.AMR1 = 0xFF;
      p_filter.AMR2 = 0xFF;
      p_filter.AMR3 = 0xFF;
      ESP32Can.CANConfigFilter(&p_filter);
    */
    //start CAN Module
    ESP32Can.CANInit();
}

void CanDevice::sendCanFrame(const CAN_frame_t *p_frame) {
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[128];
        snprintf(buf, 128, "Sending CAN frame %" PRIu32 " DLC %d, [%d, %d, %d, %d, %d, %d, %d, %d]",
                p_frame->MsgID,
                p_frame->FIR.B.DLC,
                p_frame->data.u8[0],
                p_frame->data.u8[1],
                p_frame->data.u8[2],
                p_frame->data.u8[3],
                p_frame->data.u8[4],
                p_frame->data.u8[5],
                p_frame->data.u8[6],
                p_frame->data.u8[7]);
        Logger::verbose(LOG_TAG_CANDEVICE, buf);
    }
    xSemaphoreTake(mutex_v, portMAX_DELAY);
    ESP32Can.CANWriteFrame(p_frame);
    xSemaphoreGive(mutex_v);
    delay(1); // This is needed, dunno why!
}