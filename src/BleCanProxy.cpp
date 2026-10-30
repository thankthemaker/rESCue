#include "BleCanProxy.h"
#include "vesc_packet.h"


BleCanProxy::BleCanProxy(CanDevice *candevice, Stream *stream, uint8_t vesc_id, uint8_t ble_proxy_can_id) {
    this->candevice = candevice;
    this->stream = stream;
    this->vesc_id = vesc_id;
    this->ble_proxy_can_id = ble_proxy_can_id;
}

void BleCanProxy::sendFrame(twai_message_t &frame) {
    if (candevice->sendCanFrame(&frame)) {
        framesSent++;
    }
}

void BleCanProxy::proxyIn(const std::string &in) {
    if (in.size() < 2) {
        return; // not enough for a packet start byte plus its length
    }

    const unsigned long startedAt = millis();

    if (!longPacket) {
        framesSent = 0;
        const uint8_t packetType = (uint8_t) in[0];

        switch (packetType) {
            case 2:
                if (in.size() < 3) {
                    return; // need start byte, length and the command byte
                }
                headerLength = 2;
                length = (uint8_t) in[1];
                command = (uint8_t) in[2];
                processing = true;
                // a short packet is only "long" if this chunk does not hold it completely
                if (in.size() < (size_t) headerLength + (size_t) length + 3) {
                    longPacket = true;
                }
                break;
            case 3:
                if (in.size() < 4) {
                    return;
                }
                headerLength = 3;
                length = (uint16_t) (((uint8_t) in[1] << 8) | (uint8_t) in[2]);
                command = (uint8_t) in[3];
                longPacket = true;
                processing = true;
                break;
            default:
                return;
        }
        ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "Proxy in, command %d, length %d\n", command, length);
    }

    if (length <= 6 && !longPacket) {
        // short packet -> one CAN_PACKET_PROCESS_SHORT_BUFFER frame
        // layout expected by the VESC: [id, send flag, payload...]
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 0x02 + length;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0x00;
        tx_frame.data[2] = command;
        for (int i = 3; i < length + 2; i++) {
            tx_frame.data[i] = (uint8_t) in.at(i);
        }
        sendFrame(tx_frame);
    } else {
        longPackBuffer += in;

        // a complete packet is header + payload + crc (2) + end byte (1)
        const size_t frameLength = (size_t) headerLength + (size_t) length + 3;
        if (longPackBuffer.size() < frameLength) {
            return; // wait for the remaining chunks
        }
        // keep exactly one packet, so the CRC bytes read below are always in place
        longPackBuffer = longPackBuffer.substr(headerLength, length + 3);

        unsigned int end_a = 0;
        for (unsigned int byteNum = 0; byteNum < length; byteNum += 7) {
            if (byteNum > 255) {
                break;
            }

            end_a = byteNum + 7;

            const int sendLen = (length >= byteNum + 7) ? 7 : length - byteNum;

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + vesc_id;
            tx_frame.data_length_code = sendLen + 1;
            tx_frame.data[0] = byteNum; // start byte counter of the frame

            for (int i = 1; i < sendLen + 1; i++) {
                tx_frame.data[i] = (uint8_t) longPackBuffer.at(byteNum - 1 + i);
            }
            sendFrame(tx_frame);
        }

        for (unsigned int byteNum = end_a; byteNum < length; byteNum += 6) {
            const int sendLen = (length >= byteNum + 6) ? 6 : length - byteNum;

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + vesc_id;
            tx_frame.data_length_code = sendLen + 2;
            tx_frame.data[0] = byteNum >> 8;
            tx_frame.data[1] = byteNum & 0xFF;

            for (int i = 2; i < sendLen + 2; i++) {
                tx_frame.data[i] = (uint8_t) longPackBuffer.at(byteNum - 2 + i);
            }

            sendFrame(tx_frame);
        }

        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 6;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0; // 0 -> the VESC sends a reply back to ble_proxy_can_id
        tx_frame.data[2] = length >> 8;
        tx_frame.data[3] = length & 0xFF;
        // the buffer holds payload + crc (2) + end byte (1), so the CRC sits at size-3 / size-2
        tx_frame.data[4] = longPackBuffer.at(longPackBuffer.size() - 3);
        tx_frame.data[5] = longPackBuffer.at(longPackBuffer.size() - 2);
        sendFrame(tx_frame);
    }

    if (framesSent > 0) {
        const unsigned long elapsed = millis() - startedAt;
        if (elapsed > 50) {
            ESP_LOGI(LOG_TAG_BLE_CAN_PROXY, "proxyIn: %u bytes in %u CAN frames took %lu ms",
                     (unsigned) length, framesSent, elapsed);
        } else {
            ESP_LOGD(LOG_TAG_BLE_CAN_PROXY, "proxyIn: %u bytes in %u CAN frames took %lu ms",
                     (unsigned) length, framesSent, elapsed);
        }
    }

    length = 0;
    command = 0;
    processing = false;
    longPacket = false;
    longPackBuffer.clear();
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
