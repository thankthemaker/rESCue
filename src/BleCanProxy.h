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

    /** BLE -> VESC: hand incoming client bytes over to the main loop. */
    void proxyIn(const std::string &in);

    /**
     * VESC -> BLE: write a VESC packet (framing and CRC are added here) to the
     * stream. Called from the main loop.
     */
    void proxyOut(const uint8_t *payload, unsigned int size);

    /**
     * Main loop: parse buffered client data and send the CAN transport frames.
     * All proxy work (parsing, framing, CAN I/O) happens here, never in the BLE
     * callback - a long transfer must not block the NimBLE host task.
     */
    void loop();

    /** true while a multi frame packet from the BLE client is being assembled. */
    boolean processing = false;

  private:
    void sendFrame(twai_message_t &frame);
    void transmitPacket(const uint8_t *payload, unsigned int payloadLen);

    CanDevice *candevice;
    Stream *stream;
    SemaphoreHandle_t rxMutex;   /**< guards handOff between the BLE and the loop task */
    std::string handOff;         /**< written by the BLE task, taken by loop() */
    std::string rxBuffer;        /**< main loop only: reassembly of client packets */
    std::string frameBuffer;
    unsigned long processingSince = 0;
    uint8_t vesc_id;
    uint8_t ble_proxy_can_id;
    unsigned int framesSent = 0;
};

#endif //RESCUE_BLECANPROXY_H
