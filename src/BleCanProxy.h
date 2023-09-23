#ifndef RESCUE_BLECANPROXY_H
#define RESCUE_BLECANPROXY_H

#include "Arduino.h"
#include "config.h"
#include <Logger.h>
#include "VescCanConstants.h"
#include "LoopbackStream.h"
#include "CanDevice.h"

#define LOG_TAG_BLE_CAN_PROXY "BleCanProxy"

class BleCanProxy {
  public:
    BleCanProxy(CanDevice *candevice, Stream *stream, uint8_t vesc_id, uint8_t ble_proxy_can_id);
    void proxyIn(std::string in);
    void proxyOut(uint8_t *data, unsigned int size, uint8_t crc1, uint8_t crc2);
    boolean processing = false;

  private:
    const static int bufSize = 64;
    char buf[bufSize];
    CanDevice *candevice;
    Stream *stream;
    uint16_t length = 0;
    uint8_t command = 0;
    boolean longPacket = false;
    std::string longPackBuffer;
    uint8_t vesc_id;
    uint8_t ble_proxy_can_id;
};

#endif //RESCUE_BLECANPROXY_H
