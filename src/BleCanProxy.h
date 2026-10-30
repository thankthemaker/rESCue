#ifndef RESCUE_BLECANPROXY_H
#define RESCUE_BLECANPROXY_H

#include "Arduino.h"
#include "config.h"
#include "VescCanConstants.h"
#include "LoopbackStream.h"
#include "CanDevice.h"
#include <string>

static const char* LOG_TAG_BLE_CAN_PROXY = "BleCanProxy";

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

    CanDevice *candevice;
    Stream *stream;
    uint16_t length = 0;
    uint8_t command = 0;
    int headerLength = 2;
    boolean longPacket = false;
    std::string longPackBuffer;
    std::string frameBuffer;
    uint8_t vesc_id;
    uint8_t ble_proxy_can_id;
    unsigned int framesSent = 0;
};

#endif //RESCUE_BLECANPROXY_H
