#ifndef RESCUE_BLECANPROXY_H
#define RESCUE_BLECANPROXY_H

#include "Arduino.h"
#include "config.h"
#include "VescCanConstants.h"
#include "LoopbackStream.h"
#include "CanDevice.h"
#include <string>

static const char* LOG_TAG_BLE_CAN_PROXY = "BleCanProxy";

/** Upper bound for the buffer holding incoming (possibly incomplete) client data.
 *  A framed VESC packet is at most 518 bytes, so this leaves room for batched writes. */
#define BLE_PROXY_RX_BUFFER_MAX 2048
/** Give up on a partially received packet after this long and resume polling. */
#define BLE_PROXY_PACKET_TIMEOUT_MS 2000

/**
 * Transparent bridge between the BLE client (VESC app) and the VESC on CANBUS.
 *
 * Incoming BLE data is a VESC packet; it is split into CAN "fill rx buffer"
 * frames plus a "process rx buffer" frame for the VESC. Replies from the VESC
 * are turned back into a VESC packet and written to the loopback stream that
 * feeds the BLE characteristic.
 */
class BleCanProxy {
  public:
    BleCanProxy(CanDevice *candevice, Stream *stream, uint8_t vesc_id, uint8_t ble_proxy_can_id);

    /** BLE -> VESC: translate a VESC packet into the CAN transport frames. */
    void proxyIn(const std::string &in);

    /** VESC -> BLE: write a VESC packet (framing and CRC are added here) to the stream. */
    void proxyOut(const uint8_t *payload, unsigned int size);

    /** true while a multi frame packet from the BLE client is being assembled. */
    boolean processing = false;

  private:
    void sendFrame(twai_message_t &frame);
    void transmitPacket(const uint8_t *payload, unsigned int payloadLen);

    CanDevice *candevice;
    Stream *stream;
    std::string rxBuffer;
    std::string frameBuffer;
    unsigned long processingSince = 0;
    uint8_t vesc_id;
    uint8_t ble_proxy_can_id;
    unsigned int framesSent = 0;
};

#endif //RESCUE_BLECANPROXY_H
