#ifndef RESCUE_VESC_CAN_ID_H
#define RESCUE_VESC_CAN_ID_H

#include <cstdint>

/** Mask covering the 29 bit extended CAN identifier. */
#define VESC_CAN_ID_MASK 0x1FFFFFFFU

/**
 * VESC CAN frame identifiers.
 *
 * The VESC encodes a frame as (packet_type << 8) | controller_id inside the
 * 29 bit extended CAN identifier.
 *
 * The TWAI driver masks received extended identifiers to 29 bits, so a receive
 * id must NOT have bit 31 set: a constant built with 0x8000 << 16 can never
 * match an incoming frame and the transport would silently stop decoding data.
 */
static inline uint32_t canSendId(uint16_t packet, uint8_t id) {
    return (uint32_t(0x8000) << 16) + (uint16_t(packet) << 8) + id;
}

static inline uint32_t canReceiveId(uint16_t packet, uint8_t id) {
    return (uint32_t(0x0000) << 16) + (uint16_t(packet) << 8) + id;
}

#endif //RESCUE_VESC_CAN_ID_H
