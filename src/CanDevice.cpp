#include "CanDevice.h"

boolean CanDevice::init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_CAN_TX_PIN, GPIO_CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.rx_queue_len=1000;
    g_config.tx_queue_len=10;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(LOG_TAG_CANDEVICE, "Driver installed\n");
    } else {
        ESP_LOGE(LOG_TAG_CANDEVICE, "Failed to install driver\n");
        return false;
    }
    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        ESP_LOGI(LOG_TAG_CANDEVICE, "Driver started\n");
    } else {
        ESP_LOGE(LOG_TAG_CANDEVICE, "Failed to start driver\n");
        return false;
    }
    return true;
}

boolean CanDevice::sendCanFrame(const twai_message_t *p_frame) {
    /*ESP_LOGE(LOG_TAG_CANDEVICE, "Sending CAN frame %" PRIu32 " DLC %d, [%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x]",
                p_frame->identifier,
                p_frame->data_length_code,
                p_frame->data[0],
                p_frame->data[1],
                p_frame->data[2],
                p_frame->data[3],
                p_frame->data[4],
                p_frame->data[5],
                p_frame->data[6],
                p_frame->data[7]);*/

    if (esp_log_level_get(LOG_TAG_CANDEVICE) >= ESP_LOG_DEBUG) {
        char buf[128];
        ESP_LOGD(LOG_TAG_CANDEVICE, "Sending CAN frame %" PRIu32 " DLC %d, [%d, %d, %d, %d, %d, %d, %d, %d]",
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
    }
    xSemaphoreTake(mutex_v, portMAX_DELAY);
    //Queue message for transmission
    if (twai_transmit(p_frame, pdMS_TO_TICKS(10)) != ESP_OK) {
        ESP_LOGE(LOG_TAG_CANDEVICE, "Failed to queue message for transmission\n");
        xSemaphoreGive(mutex_v);
        return false;
    }
    xSemaphoreGive(mutex_v);
    delay(1); // This is needed, dunno why!
    return true;
}