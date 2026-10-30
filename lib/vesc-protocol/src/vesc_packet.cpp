#include "vesc_packet.h"

unsigned short crc16_rolling(unsigned short cksum, const uint8_t *buf, unsigned int len) {
    for (unsigned int i = 0; i < len; i++) {
        cksum ^= (unsigned short) buf[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            if (cksum & 0x8000) {
                cksum = (unsigned short) ((cksum << 1) ^ 0x1021);
            } else {
                cksum = (unsigned short) (cksum << 1);
            }
        }
    }
    return cksum;
}

unsigned short crc16(const uint8_t *buf, unsigned int len) {
    return crc16_rolling(0, buf, len);
}

int vesc_packet_header_length(unsigned int payloadLen) {
    if (payloadLen <= 255) {
        return 2;
    }
    if (payloadLen <= 65535) {
        return 3;
    }
    return 4;
}

unsigned int vesc_packet_frame_length(unsigned int payloadLen) {
    return (unsigned int) vesc_packet_header_length(payloadLen) + payloadLen + 3;
}

int vesc_packet_encode(const uint8_t *payload, unsigned int payloadLen, uint8_t *out, unsigned int outSize) {
    if (payload == nullptr || out == nullptr || payloadLen == 0) {
        return -1;
    }

    const unsigned int total = vesc_packet_frame_length(payloadLen);
    if (total > outSize) {
        return -1;
    }

    unsigned int index = 0;
    if (payloadLen <= 255) {
        out[index++] = 2;
        out[index++] = (uint8_t) payloadLen;
    } else if (payloadLen <= 65535) {
        out[index++] = 3;
        out[index++] = (uint8_t) (payloadLen >> 8);
        out[index++] = (uint8_t) (payloadLen & 0xFF);
    } else {
        out[index++] = 4;
        out[index++] = (uint8_t) (payloadLen >> 16);
        out[index++] = (uint8_t) ((payloadLen >> 8) & 0xFF);
        out[index++] = (uint8_t) (payloadLen & 0xFF);
    }

    for (unsigned int i = 0; i < payloadLen; i++) {
        out[index++] = payload[i];
    }

    const unsigned short crc = crc16(payload, payloadLen);
    out[index++] = (uint8_t) (crc >> 8);
    out[index++] = (uint8_t) (crc & 0xFF);
    out[index++] = 3;

    return (int) index;
}

int vesc_short_buffer_payload(const uint8_t *data, unsigned int dlc, const uint8_t **payload) {
    if (data == nullptr || payload == nullptr || dlc < 2) {
        return -1;
    }
    *payload = data + 2;
    return (int) (dlc - 2);
}
