#include "BleCanProxy.h"
#include "vesc_packet.h"


BleCanProxy::BleCanProxy(CanDevice *candevice, Stream *stream, uint8_t vesc_id, uint8_t ble_proxy_can_id) {
    this->candevice = candevice;
    this->stream = stream;
    this->vesc_id = vesc_id;
    this->ble_proxy_can_id = ble_proxy_can_id;
    this->rxMutex = xSemaphoreCreateMutex();
}

void BleCanProxy::sendFrame(twai_message_t &frame) {
    if (candevice->sendCanFrame(&frame)) {
        framesSent++;
    }
}

/*
 * BLE -> VESC hand over.
 *
 * Called from the BLE callback, so it has to stay short: the bytes are only
 * buffered here, all parsing, framing and CAN I/O happens in loop() on the main
 * loop. A long transfer must not block the NimBLE host task.
 */
void BleCanProxy::proxyIn(const std::string &in) {
    if (in.empty()) {
        return;
    }
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }
    handOff += in;
    if (handOff.size() > BLE_PROXY_RX_BUFFER_MAX) {
        ESP_LOGW(LOG_TAG_BLE_CAN_PROXY, "dropping %u bytes of unresolvable client data",
                 (unsigned int) handOff.size());
        handOff.clear();
    }
    xSemaphoreGive(rxMutex);
}

/*
 * Main loop.
 *
 * The client delivers arbitrary chunks (a packet split over several writes,
 * several packets in one write, or stray bytes), so everything is accumulated
 * and every complete packet found in the buffer is forwarded to the VESC.
 * Whatever cannot be resolved is dropped after a timeout, otherwise a partial
 * packet would keep the periodic VESC polling paused forever.
 */
void BleCanProxy::loop() {
    std::string incoming;
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        handOff.swap(incoming);
        xSemaphoreGive(rxMutex);
    }
    if (!incoming.empty()) {
        rxBuffer += incoming;
    }

    if (!rxBuffer.empty()) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(rxBuffer.data());
        unsigned int offset = 0;
        VescPacketView view;
        while (vesc_packet_next(data, (unsigned int) rxBuffer.size(), &offset, &view)) {
            transmitPacket(data + offset + view.headerLength, view.payloadLength);
            offset += view.packetLength;
        }
        if (offset > 0) {
            rxBuffer.erase(0, offset);
        }
        if (rxBuffer.size() > BLE_PROXY_RX_BUFFER_MAX) {
            ESP_LOGW(LOG_TAG_BLE_CAN_PROXY, "dropping %u bytes of unresolvable client data",
                     (unsigned int) rxBuffer.size());
            rxBuffer.clear();
            processingSince = 0;
        }
    }

    if (!rxBuffer.empty()) {
        if (processingSince == 0) {
            processingSince = millis();
        } else if (millis() - processingSince > BLE_PROXY_PACKET_TIMEOUT_MS) {
            ESP_LOGW(LOG_TAG_BLE_CAN_PROXY, "dropping incomplete packet after %d ms",
                     BLE_PROXY_PACKET_TIMEOUT_MS);
            rxBuffer.clear();
            processingSince = 0;
        }
    } else {
        processingSince = 0;
    }

    processing = !rxBuffer.empty();
}

/* Translate one VESC packet into the CAN transport frames for the VESC. */
void BleCanProxy::transmitPacket(const uint8_t *payload, unsigned int payloadLen) {
    framesSent = 0;
    const unsigned long startedAt = millis();
    ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "proxy in, command %d, length %u\n",
             payloadLen > 0 ? payload[0] : 0, payloadLen);

    if (payloadLen <= 6) {
        // short packet -> one CAN_PACKET_PROCESS_SHORT_BUFFER frame.
        // Layout expected by the VESC: [controller id, send flag, payload...]
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 0x02 + payloadLen;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0x00;
        for (unsigned int i = 0; i < payloadLen; i++) {
            tx_frame.data[2 + i] = payload[i];
        }
        sendFrame(tx_frame);
    } else {
        // long packet -> CAN_PACKET_FILL_RX_BUFFER[_LONG] frames plus a process frame.
        // 7 payload bytes per frame up to offset 255, 6 bytes per frame beyond that.
        unsigned int endA = 0;
        for (unsigned int byteNum = 0; byteNum < payloadLen; byteNum += 7) {
            if (byteNum > 255) {
                break;
            }
            endA = byteNum + 7;

            const unsigned int sendLen = (payloadLen >= byteNum + 7) ? 7 : payloadLen - byteNum;

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + vesc_id;
            tx_frame.data_length_code = sendLen + 1;
            tx_frame.data[0] = (uint8_t) byteNum; // start byte counter of the frame

            for (unsigned int i = 0; i < sendLen; i++) {
                tx_frame.data[1 + i] = payload[byteNum + i];
            }
            sendFrame(tx_frame);
        }

        for (unsigned int byteNum = endA; byteNum < payloadLen; byteNum += 6) {
            const unsigned int sendLen = (payloadLen >= byteNum + 6) ? 6 : payloadLen - byteNum;

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + vesc_id;
            tx_frame.data_length_code = sendLen + 2;
            tx_frame.data[0] = (uint8_t) (byteNum >> 8);
            tx_frame.data[1] = (uint8_t) (byteNum & 0xFF);

            for (unsigned int i = 0; i < sendLen; i++) {
                tx_frame.data[2 + i] = payload[byteNum + i];
            }
            sendFrame(tx_frame);
        }

        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 6;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0; // 0 -> the VESC sends a reply back to ble_proxy_can_id
        tx_frame.data[2] = (uint8_t) (payloadLen >> 8);
        tx_frame.data[3] = (uint8_t) (payloadLen & 0xFF);
        // derive the CRC ourselves instead of trusting the trailing bytes of the packet
        const unsigned short crc = crc16(payload, payloadLen);
        tx_frame.data[4] = (uint8_t) (crc >> 8);
        tx_frame.data[5] = (uint8_t) (crc & 0xFF);
        sendFrame(tx_frame);
    }

    if (framesSent > 0) {
        const unsigned long elapsed = millis() - startedAt;
        if (elapsed > 50) {
            ESP_LOGI(LOG_TAG_BLE_CAN_PROXY, "proxyIn: %u bytes in %u CAN frames took %lu ms",
                     payloadLen, framesSent, elapsed);
        } else {
            ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "proxyIn: %u bytes in %u CAN frames took %lu ms",
                     payloadLen, framesSent, elapsed);
        }
    }
}

void BleCanProxy::proxyOut(const uint8_t *payload, unsigned int size) {
    if (payload == nullptr || size == 0) {
        return;
    }
    if (size > BUFFER_SIZE) {
        ESP_LOGE(LOG_TAG_BLE_CAN_PROXY, "proxyOut - Buffer size exceeded, abort (message not sent via proxy)");
        return;
    }

    // Framing and CRC are added here: the VESC short buffer frames carry no CRC,
    // so it has to be derived from the payload for every reply.
    const unsigned int frameLength = vesc_packet_frame_length(size);
    frameBuffer.resize(frameLength);
    const int written = vesc_packet_encode(payload, size, reinterpret_cast<uint8_t *>(&frameBuffer[0]), frameLength);
    if (written <= 0) {
        ESP_LOGE(LOG_TAG_BLE_CAN_PROXY, "proxyOut - could not encode packet, abort");
        return;
    }

    ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "Proxy out, sending %d bytes\n", size);
    const unsigned long startedAt = millis();
    stream->write(reinterpret_cast<const uint8_t *>(frameBuffer.data()), (size_t) written);
    ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "proxyOut: %u bytes took %lu ms", size, millis() - startedAt);
}
