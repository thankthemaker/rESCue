#ifndef __CANBUS_H__
#define __CANBUS_H__

#include <Arduino.h>
#include <LoopbackStream.h>
#include <vector>
#include "config.h"
#include "AppConfiguration.h"
#include "CanDevice.h"
#include "BleCanProxy.h"
#include "VescCanConstants.h"
#include "VescData.h"
#include "IVescCommunication.h"

static const char* LOG_TAG_CANBUS = "CanBus";

/**
 * VESC communication over CANBUS.
 *
 * Handles the CAN frames (status frames, command responses and the transparent
 * BLE proxy) and implements the transport specific part of IVescCommunication.
 * Decoding of command responses is delegated to the shared VescCommandParser.
 */
class CanBus : public VescCommunicationBase {
  public:
    explicit CanBus(VescData *vescData);

    void init() override;
    void toVesc(const std::string &value) override;
    void fromVesc(std::string &buffer) override;

    // BMS (OneWheel / OwIE) support, CANBUS only
    bool bmsVTOT(float vTotal, float vCharge);
    bool bmsVCell(const uint16_t *cellMillivolts, int cellMax);
    bool bmsTemps(const int8_t *thermTemps, int tempMax);
    bool bmsBal(bool isBalancing);
    bool bmsI(float currentAmps);
    bool bmsAHWH(float ampHours, float wattHours);
    bool bmsAHWHDischargeTotal(float ampHours, float wattHours);
    bool bmsAHWHChargeTotal(float ampHours, float wattHours);
    bool bmsSOCSOHTempStat(float vCellMin, float vCellMax, float soc, float soh, float cellMaxTemp,
                           bool isCharging, bool isBalancing, bool isChargeAllowed, bool isChargeOk);
    bool bmsState(bms_op_state opState, bms_fault_state faultState);

    /** true while the transparent BLE<->CAN proxy is assembling a packet. */
    bool isProxyBusy() const { return proxy != nullptr && proxy->processing; }

  private:
    void readFromVesc() override;
    bool sendFirmwareVersionRequest() override;
    bool sendRealtimeDataRequest() override;
    bool sendFloatPackageDataRequest() override;
    bool requestSlotBusy() override { return isProxyBusy(); }
    void afterRead() override;

    void processFrame(const twai_message_t &rx_frame, int frameCount);
    static int32_t readInt32Value(const twai_message_t &rx_frame, int startbyte);
    static int16_t readInt16Value(const twai_message_t &rx_frame, int startbyte);
    static void printFrame(const twai_message_t &rx_frame, int frameCount);

    /** Build a VESC CAN identifier for the given packet type and target id. */
    uint32_t canId(uint16_t packet, uint8_t id) const {
        return (uint32_t(0x8000) << 16) + (uint16_t(packet) << 8) + id;
    }
    bool sendFrame(const twai_message_t &frame) { return candevice->sendCanFrame(&frame); }

    const static int maxFramesPerLoop = 1000;
    const static size_t maxBufferSize = 4096;

    CanDevice *candevice = nullptr;
    LoopbackStream *stream = nullptr;
    BleCanProxy *proxy = nullptr;
    uint8_t vesc_id = 0;
    uint8_t esp_can_id = 0;
    uint8_t ble_proxy_can_id = 0;

    uint32_t RECV_STATUS_1 = 0;
    uint32_t RECV_STATUS_2 = 0;
    uint32_t RECV_STATUS_3 = 0;
    uint32_t RECV_STATUS_4 = 0;
    uint32_t RECV_STATUS_5 = 0;
    uint32_t RECV_FILL_RX_BUFFER = 0;
    uint32_t RECV_PROCESS_RX_BUFFER = 0;
    uint32_t RECV_PROCESS_SHORT_BUFFER_PROXY = 0;
    uint32_t RECV_FILL_RX_BUFFER_PROXY = 0;
    uint32_t RECV_FILL_RX_BUFFER_LONG_PROXY = 0;
    uint32_t RECV_PROCESS_RX_BUFFER_PROXY = 0;

    std::vector<uint8_t> buffer = {};
    std::vector<uint8_t> proxybuffer = {};
};

#endif //__CANBUS_H__
