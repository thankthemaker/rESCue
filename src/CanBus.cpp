#include "CanBus.h"

// CRC 16
// Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
// Initial value: 0xFFFF
#define POLY 0x1021
#define INIT 0x0000

// Lookup table for CRC-16
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t crc = INIT;

    while (length--) {
        crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ *data++];
    }
    return crc;
}


CanBus::CanBus(VescData *vescData) {
    this->vescData = vescData;
    this->stream = new LoopbackStream(BUFFER_SIZE);
}

void CanBus::init() {
    AppConfiguration::getInstance()->config.vescId;
    
    esp_can_id = vesc_id + 1;
    ble_proxy_can_id = vesc_id + 2;
    RECV_STATUS_1 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS) << 8) + this->vesc_id;
    RECV_STATUS_2 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_2) << 8) + this->vesc_id;
    RECV_STATUS_3 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_3) << 8) + this->vesc_id;
    RECV_STATUS_4 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_4) << 8) + vesc_id;
    RECV_STATUS_5 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_5) << 8) + vesc_id;

    RECV_FILL_RX_BUFFER = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + esp_can_id;
    RECV_PROCESS_RX_BUFFER = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + esp_can_id;

    RECV_PROCESS_SHORT_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + ble_proxy_can_id;
    RECV_FILL_RX_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + ble_proxy_can_id;
    RECV_FILL_RX_BUFFER_LONG_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + ble_proxy_can_id;
    RECV_PROCESS_RX_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + ble_proxy_can_id;
    candevice = new CanDevice();
    candevice->init();
    proxy = new BleCanProxy(candevice, stream, vesc_id, ble_proxy_can_id);
}

/*
  The VESC has to be configured to send status 1-5 regularly. It is recommenden to reduce the
  interval from 50Hz to something around 1-5Hz, which is absolutely sufficient for this application.
*/
void CanBus::loop() {
    int frameCount = 0;
    twai_message_t rx_frame;
    unsigned long now = millis();
    if (initialized) {
        if (lastRealtimeData <= now && now - lastRealtimeData > interval && !proxy->processing) {
            if(!requestRealtimeData()) {
                lastRealtimeData = (millis() + 500);
                this->vescData->connected = false;
            }
        }

        if (lastBalanceData <= now && now - lastBalanceData > interval && !proxy->processing) {
            if(!requestFloatPackageData()) {
                lastBalanceData = (millis() + 500);
                this->vescData->connected = false;
            }
        }
    } else if(initRetryCounter > 0 && lastRetry <= now && now - lastRetry > 500) {
        requestFirmwareVersion();
        initRetryCounter--;
        lastRetry = millis();
        if(initRetryCounter == 0) {
            ESP_LOGE(LOG_TAG_CANBUS, "CANBUS initialization failed");
            initRetryCounter = 1;
            lastRetry = (millis() + 5000);
        }
    }

    //receive next CAN frame from queue
    while (twai_receive( &rx_frame, 3 * portTICK_PERIOD_MS) == ESP_OK) {
        if(!this->vescData->connected) {
            this->vescData->connected = true;
        }
        if (!initialized) {
            ESP_LOGI(LOG_TAG_CANBUS, "CANBUS is now initialized");
            initialized = true;
        }
        frameCount++;
        //VESC only uses ext packages, so skip std packages
        if (rx_frame.extd) {
            if (esp_log_level_get(LOG_TAG_CANBUS) > ESP_LOG_DEBUG) {
                printFrame(rx_frame, frameCount);
            }
            processFrame(rx_frame, frameCount);
        }
        clearFrame(rx_frame);
        if (frameCount > 1000) {
            // WORKAROUND if messages arrive too fast
            ESP_LOGE(LOG_TAG_CANBUS, "reached 1000 frames in one loop, abort");
            buffer.clear();
            return;
        }
    }
    if (esp_log_level_get(LOG_TAG_CANBUS) > ESP_LOG_DEBUG) {
        dumpVescValues();
    }
}

boolean CanBus::isInitialized()
{
    return initialized;
}

int CanBus::getInterval()
{
    return interval;
}

boolean CanBus::bmsVTOT(float v_tot, float v_charge) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    
    // Prepare the payload
    buffer_append_float32_auto(buffer, v_tot, &send_index); //v_tot
    buffer_append_float32_auto(buffer, v_charge, &send_index); //v_charge

    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_V_TOT) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);

    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
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
boolean CanBus::bmsSOCSOHTempStat(float vCellMin, float vCellMax, float SOC, float SOH, float cellMaxTemp, boolean isCharging, boolean isBalancing, boolean isChargeAllowed, boolean isChargeOk) {
    twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    
    // Prepare the payload
    buffer_append_float16(buffer, vCellMin, 1e3, &send_index);
	buffer_append_float16(buffer, vCellMax, 1e3, &send_index);
	buffer[send_index++] = (uint8_t)(SOC / 100 * 255);
	buffer[send_index++] = (uint8_t)(SOH / 100 * 255);
	buffer[send_index++] = (int8_t)cellMaxTemp;
	buffer[send_index++] =
			((isCharging ? 1 : 0) << 0) |
			((isBalancing ? 1 : 0) << 1) |
			((isChargeAllowed ? 1 : 0) << 2) |
			((isChargeOk ? 1 : 0) << 3);

    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_SOC_SOH_TEMP_STAT) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);

    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsAHWHDischargeTotal(float ampHours, float wattHours) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    buffer_append_float32_auto(buffer, ampHours, &send_index);
	buffer_append_float32_auto(buffer, wattHours, &send_index);
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_AH_WH_DIS_TOTAL) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);
    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsAHWHChargeTotal(float ampHours, float wattHours) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    buffer_append_float32_auto(buffer, ampHours, &send_index);
	buffer_append_float32_auto(buffer, wattHours, &send_index);
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_AH_WH_CHG_TOTAL) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);
    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsAHWH(float ampHours, float wattHours) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    buffer_append_float32_auto(buffer, ampHours, &send_index);
	buffer_append_float32_auto(buffer, wattHours, &send_index);
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_AH_WH) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);
    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsI(float currentAmps) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
    buffer_append_float32_auto(buffer, currentAmps, &send_index);
	buffer_append_float32_auto(buffer, currentAmps, &send_index);
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_I) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);
    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsBal(boolean isBalancing) {
   twai_message_t tx_frame = {};
    int32_t send_index = 0;
    uint8_t buffer[8];  // 8 bytes for two 32-bit floats
	buffer[send_index++] = 15;

	uint64_t bal_state = 0x0;
    if (isBalancing) bal_state=0xFFFFFFFFFFFFFFFF;
	buffer[send_index++] = (bal_state >> 48) & 0xFF;
	buffer[send_index++] = (bal_state >> 40) & 0xFF;
	buffer[send_index++] = (bal_state >> 32) & 0xFF;
	buffer[send_index++] = (bal_state >> 24) & 0xFF;
	buffer[send_index++] = (bal_state >> 16) & 0xFF;
	buffer[send_index++] = (bal_state >> 8) & 0xFF;
	buffer[send_index++] = (bal_state >> 0) & 0xFF;
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_BAL) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes

    memcpy(&tx_frame.data[0], buffer, 8);
    // Send CAN frame
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::bmsVCell(const uint16_t* cellMillivolts, int cell_max) {
	int cell_now = 0;
	while (cell_now < cell_max) {
        twai_message_t tx_frame = {};
        // Configure CAN frame
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_V_CELL) << 8) + vesc_id;
        tx_frame.data_length_code = 0x8;  //sending 8 bytes
        uint8_t buffer[8];
		int32_t send_index = 0;
		buffer[send_index++] = cell_now;
		buffer[send_index++] = cell_max;
		if (cell_now < cell_max) {
			buffer_append_float16(buffer, cellMillivolts[cell_now++]/1000.0f, 1e3, &send_index);
		}
		if (cell_now < cell_max) {
			buffer_append_float16(buffer, cellMillivolts[cell_now++]/1000.0f, 1e3, &send_index);
		}
		if (cell_now < cell_max) {
			buffer_append_float16(buffer, cellMillivolts[cell_now++]/1000.0f, 1e3, &send_index);
		}
        memcpy(&tx_frame.data[0], buffer, 8);
        // Send CAN frame
        candevice->sendCanFrame(&tx_frame);
    }
    return true;
}

boolean CanBus::bmsTemps(const int8_t* thermTemps, int temp_max) {
    int temp_now = 0;
	while (temp_now < temp_max) {
        twai_message_t tx_frame = {};
        // Configure CAN frame
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_TEMPS) << 8) + vesc_id;
        tx_frame.data_length_code = 0x8;  //sending 8 bytes
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
        // Send CAN frame
        candevice->sendCanFrame(&tx_frame);		
    }
    twai_message_t tx_frame = {};
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_HUM) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes
    uint8_t buffer[8];
	int32_t send_index = 0;
	buffer_append_float16(buffer, thermTemps[4], 1e2, &send_index);
	buffer_append_float16(buffer, 0.0f, 1e2, &send_index);
	buffer_append_float16(buffer, thermTemps[4], 1e2, &send_index); // Put IC temp here instead of making mew msg
    memcpy(&tx_frame.data[0], buffer, 8);
    candevice->sendCanFrame(&tx_frame);
    return true;
}

boolean CanBus::bmsState(bms_op_state op_state, bms_fault_state fault_state) {
    twai_message_t tx_frame = {};
    // Configure CAN frame
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_BMS_STATE) << 8) + vesc_id;
    tx_frame.data_length_code = 0x8;  //sending 8 bytes
    uint8_t buffer[8];
	int32_t send_index = 0;
	buffer[send_index++] = op_state;
	buffer[send_index++] = fault_state;
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::requestFirmwareVersion() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestFirmwareVersion");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x03;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x00;  // COMM_FW_VERSION
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::requestRealtimeData() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestRealtimeData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x07;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x32;      // COMM_GET_VALUES_SELECTIVE
    // mask
    tx_frame.data[3] = 0x00;      // Byte1 of mask (Bits 24-31)
    tx_frame.data[4] = 0x00;      // Byte2 of mask (Bits 16-23)
    tx_frame.data[5] = B10000111; // Byte3 of mask (Bits 8-15)
    tx_frame.data[6] = B11000011; // Byte4 of mask (Bits 0-7)
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::requestBalanceData() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestBalanceData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x03;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x4F;  // COMM_GET_DECODED_BALANCE
    return candevice->sendCanFrame(&tx_frame);
}

boolean CanBus::requestFloatPackageData() {
    ESP_LOGI(LOG_TAG_CANBUS, "requestFloatPackageData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x05;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x24;  // COMM_CUSTOM_APP_DATA (0x24)
    tx_frame.data[3] = 0x65;  // FLOAT PACKAGE (0x65)
    tx_frame.data[4] = 0x1;  // FLOAT_COMMAND_GET_RTDATA (0x1)
    return candevice->sendCanFrame(&tx_frame);
}

void CanBus::ping() {
    twai_message_t tx_frame = {};
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PING) << 8) + vesc_id;
    tx_frame.data_length_code = 0x01;
    tx_frame.data[0] = esp_can_id;
    candevice->sendCanFrame(&tx_frame);
}

void CanBus::printFrame(twai_message_t rx_frame, int frameCount) {
    if (rx_frame.rtr)
        printf("#%d RTR from 0x%08x, DLC %d\r\n", frameCount, rx_frame.identifier, rx_frame.data_length_code);
    else {
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

void CanBus::clearFrame(twai_message_t rx_frame) {
    rx_frame.identifier=0;
    rx_frame.data_length_code=0;
    rx_frame.data[0]=0;
    rx_frame.data[1]=0;
    rx_frame.data[2]=0;
    rx_frame.data[3]=0;
    rx_frame.data[4]=0;
    rx_frame.data[5]=0;
    rx_frame.data[6]=0;
    rx_frame.data[7]=0;
}

void CanBus::processFrame(twai_message_t rx_frame, int frameCount) {
    String frametype = "";
    uint32_t ID = rx_frame.identifier;
    if (RECV_STATUS_1 == ID) {
        frametype = "status1";
        vescData->erpm = readInt32Value(rx_frame, 0);
        vescData->current = readInt16Value(rx_frame, 4) / 10.0;
        vescData->dutyCycle = readInt16Value(rx_frame, 6);
    }
     if (RECV_STATUS_2 == ID) {
        frametype = "status2";
        vescData->ampHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->ampHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    }
     if (RECV_STATUS_3 == ID) {
        frametype = "status3";
        vescData->wattHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->wattHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    }
     if (RECV_STATUS_4 == ID) {
        frametype = "status4";
        vescData->mosfetTemp = readInt16Value(rx_frame, 0) / 10.0;
        vescData->motorTemp = readInt16Value(rx_frame, 2) / 10.0;
        vescData->totalCurrentIn = readInt16Value(rx_frame, 4) / 10.0;
        vescData->pidPosition = readInt16Value(rx_frame, 6) / 50.0;
        vescData->motorPosition = readInt16Value(rx_frame, 6) / 50.0;
    }
     if (RECV_STATUS_5 == ID) {
        frametype = "status5";
        vescData->tachometer = readInt32Value(rx_frame, 0);
        vescData->inputVoltage = readInt16Value(rx_frame, 4) / 10.0;
        vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
    }

    if (RECV_PROCESS_SHORT_BUFFER_PROXY == ID) {        
        frametype = "process short buffer for <<BLE proxy>>";

        if (esp_log_level_get(LOG_TAG_CANBUS) >= ESP_LOG_DEBUG) {
            // convert output to hex
            char* Buffer = (char*) malloc(rx_frame.data_length_code*2+1);
            char* Buffer2 = Buffer;

            for (int i = 0; i < rx_frame.data_length_code; i++) {            
                Buffer2 += sprintf(Buffer2, "%02x", rx_frame.data[i]);
            }
            sprintf(Buffer2, "\0");
            ESP_LOGD(LOG_TAG_CANBUS, "RECV_PROCESS_SHORT_BUFFER_PROXY (len=%d): %s", rx_frame.data_length_code, Buffer);
        }
        
        // We get 8 bytes look at comm_can_send_buffer in VESC code, gets called in lispif_vesc_extensions.c

        // BYTE0 = can controller id 
        // BYTE1 = send param from comm_can_send_buffer (why is it 1 makes no sense?)

        // BYTE2-7 is payload:
        // BYTE2    0x24         for custrom app data
        // BYTE3    0x65/101dez  for float package
        // BYTE4-7  float package payload        

        // copy payload
        for (int i = 2; i < rx_frame.data_length_code; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
        // generate crc, cause data gets send without it
        uint16_t crc =  crc16(&rx_frame.data[2], proxybuffer.size());

        proxy->proxyOut(proxybuffer.data(), proxybuffer.size(), crc >> 8, crc & 0xff);
        proxybuffer.clear();
    }

     if (RECV_FILL_RX_BUFFER == ID) {
        frametype = "fill rx buffer";
        for (int i = 1; i < rx_frame.data_length_code; i++) {
            buffer.push_back(rx_frame.data[i]);
        }
    }

     if (RECV_FILL_RX_BUFFER_PROXY == ID || RECV_FILL_RX_BUFFER_LONG_PROXY == ID) {
        boolean longBuffer = RECV_FILL_RX_BUFFER_LONG_PROXY == ID;
        frametype = longBuffer ? "fill rx long buffer" : "fill rx buffer";
        for (int i = (longBuffer ? 2 : 1); i < rx_frame.data_length_code; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
    }

    if (RECV_PROCESS_RX_BUFFER == ID || RECV_PROCESS_RX_BUFFER_PROXY == ID) {
        boolean isProxyRequest = false;
        frametype = "process rx buffer for ";
        if (RECV_PROCESS_RX_BUFFER_PROXY == ID) {
            frametype += " <<BLE proxy>> ";
            isProxyRequest = true;
        }
        //Serial.printf("bytes %d\n", buffer.size());

        if ((!isProxyRequest && buffer.empty()) || (isProxyRequest && proxybuffer.empty())) {
            Serial.printf("buffer empty, abort");
            return;
        }
        uint8_t command = (isProxyRequest ? proxybuffer : buffer).at(0);
        if (command == 0x00) {
            frametype += "COMM_FW_VERSION";
            int offset = 1;
            vescData->majorVersion = readInt8ValueFromBuffer(0, isProxyRequest);
            vescData->minorVersion = readInt8ValueFromBuffer(1, isProxyRequest);
            vescData->name = readStringValueFromBuffer(2 + offset, 12, isProxyRequest);
        } else if (command == 0x4F) {  //0x4F = 79 DEC
            frametype += "COMM_GET_DECODED_BALANCE";
            int offset = 1;
            vescData->pidOutput = readInt32ValueFromBuffer(0 + offset, isProxyRequest) / 1000000.0;
            vescData->pitch = readInt32ValueFromBuffer(4 + offset, isProxyRequest) / 1000000.0;
            vescData->roll = readInt32ValueFromBuffer(8 + offset, isProxyRequest) / 1000000.0;
            vescData->loopTime = readInt32ValueFromBuffer(12 + offset, isProxyRequest);
            vescData->motorCurrent = readInt32ValueFromBuffer(16 + offset, isProxyRequest) / 1000000.0;
            vescData->motorPosition = readInt32ValueFromBuffer(20 + offset, isProxyRequest) / 1000000.0;
            vescData->balanceState = readInt16ValueFromBuffer(24 + offset, isProxyRequest);
            vescData->switchState = readInt16ValueFromBuffer(26 + offset, isProxyRequest);
            vescData->adc1 = readInt32ValueFromBuffer(28 + offset, isProxyRequest) / 1000000.0;
            vescData->adc2 = readInt32ValueFromBuffer(32 + offset, isProxyRequest) / 1000000.0;
            lastBalanceData = millis();
        } else if (command == 0x24) {  //0x24 = 36 DEC
            frametype += "COMM_CUSTOM_APP_DATA";
            if(readInt8ValueFromBuffer(1,isProxyRequest) == 101) //magic number
            {
                if(readInt8ValueFromBuffer(2,isProxyRequest) == 1 ) //FLOAT_COMMAND_GET_RTDATA (0x1)
                {
                    int offset = 3;
                    //FLOAT PACKAGE (0x65)
                    //FLOAT_COMMAND_GET_RTDATA (0x1)
                    //printFrame(rx_frame,frameCount);
                    //dumpVescValues();
                    // Reading floats
                    vescData->pidOutput = readFloatValueFromBuffer(0 + offset, isProxyRequest);
                    vescData->pitch = readFloatValueFromBuffer(4 + offset, isProxyRequest);
                    vescData->roll = readFloatValueFromBuffer(8 + offset, isProxyRequest);
                    //vescData->loopTime = readInt32ValueFromBuffer(12 + offset, isProxyRequest); No functional equivilent
                    //vescData->motorCurrent = readInt32ValueFromBuffer(16 + offset, isProxyRequest) / 1000000.0; Done in COMM_GET_VALUES 0x4
                    //vescData->motorPosition = readInt32ValueFromBuffer(20 + offset, isProxyRequest) / 1000000.0; Done in COMM_GET_VALUES 0x4
                    // Reading state (1 byte)
                    vescData->balanceState = readInt8ValueFromBuffer(12 + offset, isProxyRequest);
                    // Reading switch_state (1 byte)
                    uint16_t switchState = readInt8ValueFromBuffer(13 + offset, isProxyRequest);
                    // Reading adc1 and adc2 (floats)
                    vescData->adc1 = readFloatValueFromBuffer(14 + offset, isProxyRequest);
                    vescData->adc2 = readFloatValueFromBuffer(18 + offset, isProxyRequest);

                    switch(switchState)
                    {
                        case 0:
                            vescData->switchState=0;
                        break;
                        case 1:
                            vescData->switchState = (vescData->adc1 > vescData->adc2) ? 1 : 2;
                        break;
                        case 2:
                            vescData->switchState=3;
                        break;
                    }
                    lastBalanceData = millis();
                }
            }
        } else if (command == 0x32) { //0x32 = 50 DEC
            frametype += "COMM_GET_VALUES_SELECTIVE";
            int offset = 1;
            vescData->mosfetTemp = readInt16ValueFromBuffer(4 + offset, isProxyRequest) / 10.0;
            vescData->motorTemp = readInt16ValueFromBuffer(6 + offset, isProxyRequest) / 10.0;
            vescData->dutyCycle = readInt16ValueFromBuffer(8 + offset, isProxyRequest) / 1000.0;
            vescData->erpm = readInt32ValueFromBuffer(10 + offset, isProxyRequest);
            vescData->inputVoltage = readInt16ValueFromBuffer(14 + offset, isProxyRequest) / 10.0;
            vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
            vescData->tachometer = readInt32ValueFromBuffer(16 + offset, isProxyRequest);
            vescData->tachometerAbsolut = readInt32ValueFromBuffer(20 + offset, isProxyRequest);
            vescData->fault = readInt8ValueFromBuffer(24 + offset, isProxyRequest);
            lastRealtimeData = millis();
        }  else if (command == 0x33) { //0x33 = 51 DEC
            frametype += "COMM_GET_VALUES_SETUP_SELECTIVE";
            int offset = 1;
            int startbyte = 0;
            uint32_t bitmask = readInt32ValueFromBuffer(0 + offset, isProxyRequest);
            startbyte += 4;
            if(bitmask & ((uint32_t) 1 << 0)) {
                vescData->mosfetTemp = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 1)) {
                vescData->motorTemp = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 2)) {
                // current in
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 3)) {
                // current in_total
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 4)) {
                vescData->dutyCycle = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 1000.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 5)) {
                vescData->erpm = readInt32ValueFromBuffer(startbyte + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 6)) {
                // speed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 7)) {
                vescData->inputVoltage = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 8)) {
                // battery level
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 9)) {
                // amphours consumed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 10)) {
                // amphours charged
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 11)) {
                // watthours consumed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 12)) {
                // watthours charged
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 13)) {
                vescData->tachometer = readInt32ValueFromBuffer(16 + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 14)) {
                vescData->tachometerAbsolut = readInt32ValueFromBuffer(20 + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 16)) {
                vescData->fault = readInt8ValueFromBuffer(24 + offset, isProxyRequest);
                startbyte += 1;
            }
            lastRealtimeData = millis();
        } else if (command == 0x04) {
            frametype += "COMM_GET_VALUES";
            int offset = 1;
            vescData->mosfetTemp = readInt16ValueFromBuffer(0 + offset, isProxyRequest) / 10.0;
            vescData->motorTemp = readInt16ValueFromBuffer(2 + offset, isProxyRequest) / 10.0;
            vescData->motorCurrent = readInt32ValueFromBuffer(4 + offset, isProxyRequest) / 100.0;
            vescData->current = readInt32ValueFromBuffer(8 + offset, isProxyRequest) / 100.0;
            // id = vescData->readInt32ValueFromBuffer(12 + offset, isProxyRequest) / 100.0;
            // iq = vescData->readInt32ValueFromBuffer(16 + offset, isProxyRequest) / 100.0;
            vescData->dutyCycle = readInt16ValueFromBuffer(20 + offset, isProxyRequest) / 1000.0;
            vescData->erpm = readInt32ValueFromBuffer(22 + offset, isProxyRequest);
            vescData->inputVoltage = readInt16ValueFromBuffer(26 + offset, isProxyRequest) / 10.0;
            vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
            vescData->ampHours =  readInt32ValueFromBuffer(28 + offset, isProxyRequest) / 10000.0;
            vescData->ampHoursCharged = readInt32ValueFromBuffer(32 + offset, isProxyRequest) / 10000.0;
            vescData->wattHours =  readInt32ValueFromBuffer(46 + offset, isProxyRequest) / 10000.0;
            vescData->wattHoursCharged = readInt32ValueFromBuffer(40 + offset, isProxyRequest) / 10000.0;
            vescData->tachometer = readInt32ValueFromBuffer(44 + offset, isProxyRequest);
            vescData->tachometerAbsolut = readInt32ValueFromBuffer(58 + offset, isProxyRequest);
            vescData->fault = readInt8ValueFromBuffer(52 + offset, isProxyRequest);
            lastRealtimeData = millis();
        } else if (command == 0x0E) {  //0x0E = 14 DEC
            frametype += "COMM_GET_MCCONF";
        } else if (command == 0x11) {  //0x11 = 17 DEC
            frametype += "COMM_GET_APPCONF";
        } else if (command == 0x2F) { //0x2F = 47 DEC
            frametype += "COMM_GET_VALUES_SETUP";
        } else if (command == 0x33) { //0x33 = 51 DEC
            frametype += "COMM_GET_VALUES_SETUP_SELECTIVE";
        } else if (command == 0x41) { //0x41 = 65 DEC
            frametype += "COMM_GET_IMU_DATA";
        } else if (command == 0x3E) { //0x3E = 62 DEC
            frametype += "COMM_PING_CAN";
        } else {
            frametype += command;
        }
        if (isProxyRequest) {
            proxy->proxyOut(proxybuffer.data(), proxybuffer.size(), rx_frame.data[4], rx_frame.data[5]);
            proxybuffer.clear();    
        } else {
            buffer.clear();
        }
    }

    if (esp_log_level_get(LOG_TAG_CANBUS) >= ESP_LOG_DEBUG) {
        ESP_LOGD(LOG_TAG_CANBUS, "processed frame #%d, type %s", frameCount, frametype.c_str());
    }    
}

void CanBus::dumpVescValues() {
    if (esp_log_level_get(LOG_TAG_CANBUS) < ESP_LOG_DEBUG || millis() - lastDump < 1000) {
        return;
    }
    std::string bufferString;
    bufferString += "name=";
    snprintf(buf, bufSize, "%s, ", vescData->name.c_str());
    bufferString += buf;
    bufferString += "dutycycle=";
    snprintf(buf, bufSize, "%f", vescData->dutyCycle);
    bufferString += buf;
    bufferString += ", erpm=";
    snprintf(buf, bufSize, "%f", vescData->erpm);
    bufferString += buf;
    bufferString += ", current=";
    snprintf(buf, bufSize, "%f", vescData->current);
    bufferString += buf;
    bufferString += ", ampHours=";
    snprintf(buf, bufSize, "%f", vescData->ampHours);
    bufferString += buf;
    bufferString += ", ampHoursCharged=";
    snprintf(buf, bufSize, "%f", vescData->ampHoursCharged);
    bufferString += buf;
    bufferString += ", wattHours=";
    snprintf(buf, bufSize, "%f", vescData->wattHours);
    bufferString += buf;
    bufferString += ", wattHoursCharged=";
    snprintf(buf, bufSize, "%f", vescData->wattHoursCharged);
    bufferString += buf;
    bufferString += ", mosfetTemp=";
    snprintf(buf, bufSize, "%f", vescData->mosfetTemp);
    bufferString += buf;
    bufferString += ", motorTemp=";
    snprintf(buf, bufSize, "%f", vescData->motorTemp);
    bufferString += buf;
    bufferString += ", inputVoltage=";
    snprintf(buf, bufSize, "%f", vescData->inputVoltage);
    bufferString += buf;
    bufferString += ", tachometer=";
    snprintf(buf, bufSize, "%f", vescData->tachometer);
    bufferString += buf;
    bufferString += ", pidOutput=";
    snprintf(buf, bufSize, "%f", vescData->pidOutput);
    bufferString += buf;
    bufferString += ", pitch=";
    snprintf(buf, bufSize, "%f", vescData->pitch);
    bufferString += buf;
    bufferString += ", roll=";
    snprintf(buf, bufSize, "%f", vescData->roll);
    bufferString += buf;
    bufferString += ", loopTime=";
    snprintf(buf, bufSize, "%d", vescData->loopTime);
    bufferString += buf;
    bufferString += ", motorCurrent=";
    snprintf(buf, bufSize, "%f", vescData->motorCurrent);
    bufferString += buf;
    bufferString += ", motorPosition=";
    snprintf(buf, bufSize, "%f", vescData->motorPosition);
    bufferString += buf;
    bufferString += ", balanceState=";
    snprintf(buf, bufSize, "%d", vescData->balanceState);
    bufferString += buf;
    bufferString += ", switchState=";
    snprintf(buf, bufSize, "%d", vescData->switchState);
    bufferString += buf;
    bufferString += ", adc1=";
    snprintf(buf, bufSize, "%f", vescData->adc1);
    bufferString += buf;
    bufferString += ", adc2=";
    snprintf(buf, bufSize, "%f", vescData->adc2);
    bufferString += buf;
    bufferString += ", fault=";
    snprintf(buf, bufSize, "%d", vescData->fault);
    bufferString += buf;
    ESP_LOGD(LOG_TAG_CANBUS, "%s", bufferString.c_str());
    lastDump = millis();
}

float CanBus::readFloatValueFromBuffer(int startbyte, boolean isProxyRequest) {
    int32_t index = startbyte;
    const uint8_t *currBuffer = isProxyRequest ? proxybuffer.data() : buffer.data();

    return buffer_get_float32_auto(currBuffer, &index);
}

int32_t CanBus::readInt32Value(twai_message_t rx_frame, int startbyte) {
    int32_t intVal = (
            ((int32_t) rx_frame.data[startbyte] << 24) +
            ((int32_t) rx_frame.data[startbyte + 1] << 16) +
            ((int32_t) rx_frame.data[startbyte + 2] << 8) +
            ((int32_t) rx_frame.data[startbyte + 3]));
    return intVal;
}

int16_t CanBus::readInt16Value(twai_message_t rx_frame, int startbyte) {
    int16_t intVal = (
            ((int16_t) rx_frame.data[startbyte] << 8) +
            ((int16_t) rx_frame.data[startbyte + 1]));
    return intVal;
}

int32_t CanBus::readInt32ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    int32_t intVal = (
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte) << 24) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 1) << 16) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 2) << 8) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 3)));
    return intVal;
}

int16_t CanBus::readInt16ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    int16_t intVal = (
            ((int16_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte) << 8) +
            ((int16_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 1)));
    return intVal;
}

int8_t CanBus::readInt8ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    return (isProxyRequest ? proxybuffer : buffer).at(startbyte);
}

std::string CanBus::readStringValueFromBuffer(int startbyte, int length, boolean isProxyRequest) {
    std::string name;
    for (int i = startbyte; i < startbyte + length; i++) {
        name += (char) (isProxyRequest ? proxybuffer : buffer).at(i);
    }
    return name;
}