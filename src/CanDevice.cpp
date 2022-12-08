#include "CanDevice.h"

void CanDevice::init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_26, GPIO_NUM_27, TWAI_MODE_NORMAL);
    g_config.rx_queue_len=1000;
    g_config.tx_queue_len=10;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }
    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }
}

void CanDevice::sendCanFrame(const twai_message_t *p_frame) {
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[128];
        snprintf(buf, 128, "Sending CAN frame %" PRIu32 " DLC %d, [%d, %d, %d, %d, %d, %d, %d, %d]",
                p_frame->identifier,
                p_frame->data_length_code,
                p_frame->data[0],
                p_frame->data[1],
                p_frame->data[2],
                p_frame->data[3],
                p_frame->data[4],
                p_frame->data[5],
                p_frame->data[6],
                p_frame->data[7]);
        Logger::verbose(LOG_TAG_CANDEVICE, buf);
    }
    xSemaphoreTake(mutex_v, portMAX_DELAY);
    //Queue message for transmission
    if (twai_transmit(p_frame, pdMS_TO_TICKS(10)) != ESP_OK) {
        printf("Failed to queue message for transmission\n");
    }
    xSemaphoreGive(mutex_v);
    delay(1); // This is needed, dunno why!
}