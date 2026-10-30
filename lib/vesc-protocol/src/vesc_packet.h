#ifndef VESC_PACKET_H_
#define VESC_PACKET_H_

#include <stdint.h>

/*
 * VESC packet framing and CRC helpers.
 *
 * Mirrors the framing of the VESC firmware (packet.c / util/crc.c) so the BLE
 * proxy can produce packets the app and the VESC accept:
 *   [start] [length] [payload...] [crc_hi] [crc_lo] [0x03]
 * where the CRC is crc16 over the payload only and the length covers the payload.
 */

/** CRC16-CCITT / XMODEM (polynomial 0x1021, initial value 0), as used by the VESC. */
unsigned short crc16(const uint8_t *buf, unsigned int len);
unsigned short crc16_rolling(unsigned short cksum, const uint8_t *buf, unsigned int len);

/** Number of framing bytes in front of the payload (1 start byte + 1/2/3 length bytes). */
int vesc_packet_header_length(unsigned int payloadLen);

/** Total framed length: header + payload + crc (2) + end byte (1). */
unsigned int vesc_packet_frame_length(unsigned int payloadLen);

/**
 * Encode a payload into a VESC packet.
 * @return number of bytes written to out, or -1 if it does not fit
 */
int vesc_packet_encode(const uint8_t *payload, unsigned int payloadLen, uint8_t *out, unsigned int outSize);

/**
 * Payload of a CAN "short buffer" frame, which is laid out as
 *   data[0] = controller id, data[1] = send flag, data[2..] = payload
 * (see comm_can.c, CAN_PACKET_PROCESS_SHORT_BUFFER). Note that such a frame
 * carries no CRC - it has to be derived from the payload.
 * @return payload length, or -1 if the frame is too short
 */
int vesc_short_buffer_payload(const uint8_t *data, unsigned int dlc, const uint8_t **payload);

#endif //VESC_PACKET_H_
