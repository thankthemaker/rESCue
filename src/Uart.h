#ifndef __UART_H__
#define __UART_H__

#include <Arduino.h>
#include <HardwareSerial.h>
#include <string>
#include <vector>
#include "config.h"
#include "VescData.h"
#include "IVescCommunication.h"

static const char* LOG_TAG_UART = "Uart";

// Pin fallbacks so this file also compiles for CANBUS-only boards which do not define them.
#ifndef VESC_RX_PIN
  #define VESC_RX_PIN 16
#endif
#ifndef VESC_TX_PIN
  #define VESC_TX_PIN 17
#endif

/** Upper bound for the receive buffer used to reassemble VESC frames. */
#define VESC_UART_RX_BUFFER_MAX 4096
/** Upper bound for the VESC->BLE bridge buffer. */
#define VESC_UART_BRIDGE_BUFFER_MAX 8192

class Uart : public VescCommunicationBase {
  public:
    explicit Uart(VescData *vescData);

    void init() override;
    void toVesc(const std::string &value) override;
    void fromVesc(std::string &buffer) override;

  private:
    HardwareSerial serial;
    std::vector<uint8_t> rxBuffer;
    std::string bridgeBuffer;

    void readFromVesc() override;
    bool sendFirmwareVersionRequest() override;
    bool sendRealtimeDataRequest() override;
    bool sendFloatPackageDataRequest() override;

    void parseFrames();
    void sendRaw(const std::vector<uint8_t> &frame);
};

#endif //__UART_H__
