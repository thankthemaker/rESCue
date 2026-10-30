#include "CanBus.h"
#include "buffer.h"

CanBus::CanBus(VescData *vescData) : VescCommunicationBase(vescData) {
    this->stream = new LoopbackStream(BUFFER_SIZE);
}

void CanBus::init() {
    vesc_id = AppConfiguration::getInstance()->config.vescId;
    esp_can_id = vesc_id + 1;
    ble_proxy_can_id = vesc_id + 2;

    RECV_STATUS_1 = canId(CAN_PACKET_STATUS, vesc_id);
    RECV_STATUS_2 = canId(CAN_PACKET_STATUS_2, vesc_id);
    RECV_STATUS_3 = canId(CAN_PACKET_STATUS_3, vesc_id);
    RECV_STATUS_4 = canId(CAN_PACKET_STATUS_4, vesc_id);
    RECV_STATUS_5 = canId(CAN_PACKET_STATUS_5, vesc_id);

    RECV_FILL_RX_BUFFER = canId(CAN_PACKET_FILL_RX_BUFFER, esp_can_id);
    RECV_PROCESS_RX_BUFFER = canId(CAN_PACKET_PROCESS_RX_BUFFER, esp_can_id);

    RECV_PROCESS_SHORT_BUFFER_PROXY = canId(CAN_PACKET_PROCESS_SHORT_BUFFER, ble_proxy_can_id);
    RECV_FILL_RX_BUFFER_PROXY = canId(CAN_PACKET_FILL_RX_BUFFER, ble_proxy_can_id);
    RECV_FILL_RX_BUFFER_LONG_PROXY = canId(CAN_PACKET_FILL_RX_BUFFER_LONG, ble_proxy_can_id);
    RECV_PROCESS_RX_BUFFER_PROXY = canId(CAN_PACKET_PROCESS_RX_BUFFER, ble_proxy_can_id);

    candevice = new CanDevice();
    candevice->init();
    proxy = new BleCanProxy(candevice, stream, vesc_id, ble_proxy_can_id);
}

void CanBus::toVesc(const std::string &value) {
    this->proxy->proxyIn(value);
}

void CanBus::fromVesc(std::string &buffer) {
    while (stream->available()) {
        buffer.push_back((char) stream->read());
    }
}

void CanBus::afterRead() {
    if (esp_log_level_get(LOG_TAG_CANBUS) > ESP_LOG_DEBUG) {
        dumpVescValues();
    }
}

/*
 * The VESC has to be configured to send status 1-5 regularly. It is recommended to reduce the
 * interval from 50Hz to something around 1-5Hz, which is absolutely sufficient for this application.
 */
void CanBus::readFromVesc() {
    int frameCount = 0;
    twai_message_t rx_frame;

    // receive all pending CAN frames from the queue (non blocking)
    while (twai_receive(&rx_frame, 0) == ESP_OK) {
        if (!vescData->connected) {
            vescData->connected = true;
        }
        if (!initialized) {
            ESP_LOGI(LOG_TAG_CANBUS, "CANBUS is now initialized");
            initialized = true;
        }
        frameCount++;

        // VESC only uses ext packages, so skip std packages
        if (rx_frame.extd) {
            if (esp_log_level_get(LOG_TAG_CANBUS) > ESP_LOG_DEBUG) {
                printFrame(rx_frame, frameCount);
            }
            processFrame(rx_frame, frameCount);
        }

        if (frameCount > maxFramesPerLoop) {
            // WORKAROUND if messages arrive too fast
            ESP_LOGE(LOG_TAG_CANBUS, "reached %d frames in one loop, aborting", maxFramesPerLoop);
            buffer.clear();
            proxybuffer.clear();
            break;
        }
    }
}

boolean CanBus::bmsVTOT(float v_tot, float v_charge) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats

    buffer_append_float32_auto(buffer, v_tot, &send_index);    // v_tot
    buffer_append_float32_auto(buffer, v_charge, &send_index); // v_charge

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_V_TOT, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

/*
 * CAN_PACKET_BMS_SOC_SOH_TEMP_STAT
 *
 * b[0] - b[1]: V_CELL_MIN (mV)
 * b[2] - b[3]: V_CELL_MAX (mV)
 * b[4]: SoC (0 - 255)
 * b[5]: SoH (0 - 255)
 * b[6]: T_CELL_MAX (-128 to +127 degC)
 * b[7]: State bitfield:
 * [B7      B6      B5      B4      B3      B2      B1      B0      ]
 * [RSV     RSV     RSV     RSV     RSV     CHG_OK  IS_BAL  IS_CHG  ]
 */
boolean CanBus::bmsSOCSOHTempStat(float vCellMin, float vCellMax, float SOC, float SOH, float cellMaxTemp,
                                  boolean isCharging, boolean isBalancing, boolean isChargeAllowed,
                                  boolean isChargeOk) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    buffer_append_float16(buffer, vCellMin, 1e3, &send_index);
    buffer_append_float16(buffer, vCellMax, 1e3, &send_index);
    buffer[send_index++] = (uint8_t) (SOC / 100 * 255);
    buffer[send_index++] = (uint8_t) (SOH / 100 * 255);
    buffer[send_index++] = (int8_t) cellMaxTemp;
    buffer[send_index++] =
            ((isCharging ? 1 : 0) << 0) |
            ((isBalancing ? 1 : 0) << 1) |
            ((isChargeAllowed ? 1 : 0) << 2) |
            ((isChargeOk ? 1 : 0) << 3);

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_SOC_SOH_TEMP_STAT, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsAHWHDischargeTotal(float ampHours, float wattHours) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    buffer_append_float32_auto(buffer, ampHours, &send_index);
    buffer_append_float32_auto(buffer, wattHours, &send_index);

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_AH_WH_DIS_TOTAL, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsAHWHChargeTotal(float ampHours, float wattHours) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    buffer_append_float32_auto(buffer, ampHours, &send_index);
    buffer_append_float32_auto(buffer, wattHours, &send_index);

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_AH_WH_CHG_TOTAL, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsAHWH(float ampHours, float wattHours) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    buffer_append_float32_auto(buffer, ampHours, &send_index);
    buffer_append_float32_auto(buffer, wattHours, &send_index);

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_AH_WH, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsI(float currentAmps) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    // the VESC BMS packet carries both the current and the charge current
    buffer_append_float32_auto(buffer, currentAmps, &send_index);
    buffer_append_float32_auto(buffer, currentAmps, &send_index);

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_I, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsBal(boolean isBalancing) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];

    buffer[send_index++] = 15;
    uint64_t bal_state = isBalancing ? 0xFFFFFFFFFFFFFFFFULL : 0x0;
    for (int shift = 48; shift >= 0; shift -= 8) {
        buffer[send_index++] = (bal_state >> shift) & 0xFF;
    }

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_BAL, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::bmsVCell(const uint16_t *cellMillivolts, int cell_max) {
    int cell_now = 0;
    while (cell_now < cell_max) {
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = canId(CAN_PACKET_BMS_V_CELL, vesc_id);
        tx_frame.data_length_code = 0x8;

        uint8_t buffer[8];
        int32_t send_index = 0;
        buffer[send_index++] = cell_now;
        buffer[send_index++] = cell_max;
        if (cell_now < cell_max) {
            buffer_append_float16(buffer, cellMillivolts[cell_now++] / 1000.0f, 1e3, &send_index);
        }
        if (cell_now < cell_max) {
            buffer_append_float16(buffer, cellMillivolts[cell_now++] / 1000.0f, 1e3, &send_index);
        }
        if (cell_now < cell_max) {
            buffer_append_float16(buffer, cellMillivolts[cell_now++] / 1000.0f, 1e3, &send_index);
        }
        memcpy(&tx_frame.data[0], buffer, 8);
        sendFrame(tx_frame);
    }
    return true;
}

boolean CanBus::bmsTemps(const int8_t *thermTemps, int temp_max) {
    int temp_now = 0;
    while (temp_now < temp_max) {
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = canId(CAN_PACKET_BMS_TEMPS, vesc_id);
        tx_frame.data_length_code = 0x8;

        uint8_t buffer[8];
        int32_t send_index = 0;
        buffer[send_index++] = temp_now;
        buffer[send_index++] = temp_max;
        if (temp_now < temp_max) {
            buffer_append_float16(buffer, thermTemps[temp_now++], 1e2, &send_index);
        }
        if (temp_now < temp_max) {
            buffer_append_float16(buffer, thermTemps[temp_now++], 1e2, &send_index);
        }
        if (temp_now < temp_max) {
            buffer_append_float16(buffer, thermTemps[temp_now++], 1e2, &send_index);
        }
        memcpy(&tx_frame.data[0], buffer, 8);
        sendFrame(tx_frame);
    }

    if (temp_max > 4) {
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = canId(CAN_PACKET_BMS_HUM, vesc_id);
        tx_frame.data_length_code = 0x8;

        uint8_t buffer[8];
        int32_t send_index = 0;
        buffer_append_float16(buffer, thermTemps[4], 1e2, &send_index);
        buffer_append_float16(buffer, 0.0f, 1e2, &send_index);
        buffer_append_float16(buffer, thermTemps[4], 1e2, &send_index); // put IC temp here instead of making new msg
        memcpy(&tx_frame.data[0], buffer, 8);
        sendFrame(tx_frame);
    }
    return true;
}

boolean CanBus::bmsState(bms_op_state op_state, bms_fault_state fault_state) {
    twai_message_t tx_frame = {};
    uint8_t buffer[8] = {0};
    buffer[0] = op_state;
    buffer[1] = fault_state;

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_BMS_STATE, vesc_id);
    tx_frame.data_length_code = 0x8;
    memcpy(&tx_frame.data[0], buffer, 8);

    return sendFrame(tx_frame);
}

boolean CanBus::sendFirmwareVersionRequest() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestFirmwareVersion");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_PROCESS_SHORT_BUFFER, vesc_id);
    tx_frame.data_length_code = 0x03;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x00;  // COMM_FW_VERSION
    return sendFrame(tx_frame);
}

boolean CanBus::sendRealtimeDataRequest() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestRealtimeData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_PROCESS_SHORT_BUFFER, vesc_id);
    tx_frame.data_length_code = 0x07;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x32;      // COMM_GET_VALUES_SELECTIVE
    // mask selecting mosfetTemp, motorTemp, dutyCycle, erpm, inputVoltage, tachometer, fault
    tx_frame.data[3] = 0x00;      // bits 24-31
    tx_frame.data[4] = 0x00;      // bits 16-23
    tx_frame.data[5] = 0x87;      // bits 8-15
    tx_frame.data[6] = 0xC3;      // bits 0-7
    return sendFrame(tx_frame);
}

boolean CanBus::sendFloatPackageDataRequest() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestFloatPackageData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = canId(CAN_PACKET_PROCESS_SHORT_BUFFER, vesc_id);
    tx_frame.data_length_code = 0x05;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x24;  // COMM_CUSTOM_APP_DATA
    tx_frame.data[3] = 0x65;  // FLOAT_PACKAGE
    tx_frame.data[4] = 0x1;   // FLOAT_COMMAND_GET_RTDATA
    return sendFrame(tx_frame);
}

void CanBus::printFrame(const twai_message_t &rx_frame, int frameCount) {
    if (rx_frame.rtr) {
        printf("#%d RTR from 0x%08x, DLC %d\r\n", frameCount, rx_frame.identifier, rx_frame.data_length_code);
    } else {
        printf("#%d from 0x%08x, DLC %d, data [", frameCount, rx_frame.identifier, rx_frame.data_length_code);
        for (int i = 0; i < 8; i++) {
            printf("%d", (uint8_t) rx_frame.data[i]);
            if (i != 7) {
                printf("\t");
            }
        }
        printf("]\n");
    }
}

void CanBus::processFrame(const twai_message_t &rx_frame, int frameCount) {
    const uint32_t ID = rx_frame.identifier;
    std::string frametype = "";

    if (RECV_STATUS_1 == ID) {
        frametype = "status1";
        vescData->erpm = readInt32Value(rx_frame, 0);
        vescData->current = readInt16Value(rx_frame, 4) / 10.0;
        vescData->dutyCycle = readInt16Value(rx_frame, 6);
    } else if (RECV_STATUS_2 == ID) {
        vescData->ampHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->ampHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    } else if (RECV_STATUS_3 == ID) {
        vescData->wattHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->wattHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    } else if (RECV_STATUS_4 == ID) {
        vescData->mosfetTemp = readInt16Value(rx_frame, 0) / 10.0;
        vescData->motorTemp = readInt16Value(rx_frame, 2) / 10.0;
        vescData->totalCurrentIn = readInt16Value(rx_frame, 4) / 10.0;
        vescData->pidPosition = readInt16Value(rx_frame, 6) / 50.0;
        vescData->motorPosition = readInt16Value(rx_frame, 6) / 50.0;
    } else if (RECV_STATUS_5 == ID) {
        vescData->tachometer = readInt32Value(rx_frame, 0);
        vescData->inputVoltage = readInt16Value(rx_frame, 4) / 10.0;
        vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
    } else if (RECV_PROCESS_SHORT_BUFFER_PROXY == ID) {
        frametype = "process short buffer for <<BLE proxy>>";
        for (int i = 1; i < rx_frame.data_length_code && proxybuffer.size() < maxBufferSize; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
        if (rx_frame.data_length_code >= 6) {
            proxy->proxyOut(proxybuffer.data(), proxybuffer.size(), rx_frame.data[4], rx_frame.data[5]);
        }
        proxybuffer.clear();
    } else if (RECV_FILL_RX_BUFFER == ID) {
        frametype = "fill rx buffer";
        for (int i = 1; i < rx_frame.data_length_code && buffer.size() < maxBufferSize; i++) {
            buffer.push_back(rx_frame.data[i]);
        }
    } else if (RECV_FILL_RX_BUFFER_PROXY == ID || RECV_FILL_RX_BUFFER_LONG_PROXY == ID) {
        const bool longBuffer = RECV_FILL_RX_BUFFER_LONG_PROXY == ID;
        frametype = longBuffer ? "fill rx long buffer" : "fill rx buffer";
        for (int i = (longBuffer ? 2 : 1); i < rx_frame.data_length_code && proxybuffer.size() < maxBufferSize; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
    } else if (RECV_PROCESS_RX_BUFFER == ID || RECV_PROCESS_RX_BUFFER_PROXY == ID) {
        const bool isProxyRequest = RECV_PROCESS_RX_BUFFER_PROXY == ID;
        frametype = "process rx buffer";
        std::vector<uint8_t> &pending = isProxyRequest ? proxybuffer : buffer;

        if (pending.empty()) {
            ESP_LOGW(LOG_TAG_CANBUS, "process rx buffer for empty buffer, abort");
            return;
        }

        frametype = vescCommandParser->parseCommand(pending.data(), pending.size(), vescData,
                                                    AppConfiguration::getInstance()->config.batteryDrift);

        if (isProxyRequest) {
            if (rx_frame.data_length_code >= 6) {
                proxy->proxyOut(pending.data(), pending.size(), rx_frame.data[4], rx_frame.data[5]);
            }
            proxybuffer.clear();
        } else {
            buffer.clear();
        }
    }

    if (esp_log_level_get(LOG_TAG_CANBUS) >= ESP_LOG_DEBUG) {
        ESP_LOGD(LOG_TAG_CANBUS, "processed frame #%d, type %s", frameCount, frametype.c_str());
    }
}

int32_t CanBus::readInt32Value(const twai_message_t &rx_frame, int startbyte) {
    if (startbyte + 4 > rx_frame.data_length_code) {
        return 0;
    }
    return (int32_t) (((uint32_t) rx_frame.data[startbyte] << 24) |
                      ((uint32_t) rx_frame.data[startbyte + 1] << 16) |
                      ((uint32_t) rx_frame.data[startbyte + 2] << 8) |
                      (uint32_t) rx_frame.data[startbyte + 3]);
}

int16_t CanBus::readInt16Value(const twai_message_t &rx_frame, int startbyte) {
    if (startbyte + 2 > rx_frame.data_length_code) {
        return 0;
    }
    return (int16_t) (((uint16_t) rx_frame.data[startbyte] << 8) |
                      (uint16_t) rx_frame.data[startbyte + 1]);
}
