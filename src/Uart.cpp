#include "Uart.h"
#include "AppConfiguration.h"

Uart::Uart(VescData *vescData) : VescCommunicationBase(vescData), serial(2) {}

void Uart::init() {
    serial.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);
}

void Uart::toVesc(const std::string &value) {
    for (char c : value) {
        serial.write((uint8_t) c);
    }
}

void Uart::fromVesc(std::string &buffer) {
    buffer += bridgeBuffer;
    bridgeBuffer.clear();
}

/*
 * Reads everything that arrived on the UART, buffers it for the transparent BLE
 * bridge and reassembles complete VESC frames for the command parser.
 */
void Uart::readFromVesc() {
    if (serial.available() <= 0) {
        return;
    }

    if (!vescData->connected) {
        vescData->connected = true;
    }
    if (!initialized) {
        ESP_LOGI(LOG_TAG_UART, "UART is now initialized");
        initialized = true;
    }

    while (serial.available()) {
        const int byte = serial.read();
        if (byte < 0) {
            break;
        }
        bridgeBuffer.push_back((char) byte);
        if (rxBuffer.size() < VESC_UART_RX_BUFFER_MAX) {
            rxBuffer.push_back((uint8_t) byte);
        }
    }

    if (bridgeBuffer.length() > VESC_UART_BRIDGE_BUFFER_MAX) {
        ESP_LOGW(LOG_TAG_UART, "dropping %d stale bytes from the VESC bridge buffer", (int) bridgeBuffer.length());
        bridgeBuffer.clear();
    }
    parseFrames();
}

/*
 * Extracts complete VESC frames (start byte, length, payload, crc, end byte)
 * from the receive buffer and hands the payload to the command parser.
 * Incomplete frames stay in the buffer until the next call.
 */
void Uart::parseFrames() {
    size_t index = 0;
    while (index + 3 <= rxBuffer.size()) {
        const uint8_t frameType = rxBuffer[index];
        size_t headerLength;
        size_t payloadLength;

        if (frameType == 0x02) {            // short frame
            headerLength = 2;
            payloadLength = rxBuffer[index + 1];
        } else if (frameType == 0x03) {     // long frame
            if (index + 4 > rxBuffer.size()) {
                break;
            }
            headerLength = 3;
            payloadLength = ((size_t) rxBuffer[index + 1] << 8) | (size_t) rxBuffer[index + 2];
        } else {                            // not a start byte, resynchronise
            index++;
            continue;
        }

        const size_t frameLength = headerLength + payloadLength + 3; // + crc (2) + end byte (1)
        if (index + frameLength > rxBuffer.size()) {
            break;                          // frame is not complete yet
        }

        vescCommandParser->parseCommand(rxBuffer.data() + index + headerLength, payloadLength, vescData,
                                        AppConfiguration::getInstance()->config.batteryDrift, millis());
        index += frameLength;
    }

    if (index > 0) {
        rxBuffer.erase(rxBuffer.begin(), rxBuffer.begin() + index);
    }
}

void Uart::sendRaw(const std::vector<uint8_t> &frame) {
    serial.write(frame.data(), frame.size());
}

bool Uart::sendFirmwareVersionRequest() {
    ESP_LOGI(LOG_TAG_UART, "requestFirmwareVersion");
    sendRaw({0x02, 0x01, 0x00, 0x00, 0x00, 0x03});
    return true;
}

bool Uart::sendRealtimeDataRequest() {
    ESP_LOGI(LOG_TAG_UART, "requestRealtimeData");
    sendRaw({0x02, 0x05, 0x33, 0x00, 0x01, 0xff, 0xff, 0xc8, 0x03, 0x03});
    return true;
}

bool Uart::sendFloatPackageDataRequest() {
    ESP_LOGI(LOG_TAG_UART, "requestFloatPackageData");
    sendRaw({0x02, 0x03, 0x24, 0x65, 0x01, 0xbe, 0xf8, 0x03});
    return true;
}
