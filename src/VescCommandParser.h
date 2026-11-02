#ifndef __VESCCOMMANDPARSER_H__
#define __VESCCOMMANDPARSER_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include "VescData.h"

/**
 * Parses a raw VESC command response into the shared VescData struct.
 *
 * Transport independent: the caller passes a pointer to the payload (without
 * any framing) and its length. All reads are bounds checked, so a truncated or
 * corrupt frame can never read out of bounds - it simply yields a zero value
 * for the fields that are missing.
 */
class VescCommandParser {
  public:
    VescCommandParser() = default;

    /**
     * @param data        payload, starting with the VESC command byte
     * @param length      number of valid bytes in data
     * @param vescData    destination for the decoded values
     * @param batteryDrift voltage correction added to the measured input voltage
     * @param now         current time in milliseconds, used to stamp received data
     * @return a human readable name of the handled command (for logging)
     */
    std::string parseCommand(const uint8_t *data, size_t length, VescData *vescData, double batteryDrift,
                             unsigned long now);

    unsigned long getLastRealtimeDataTimestamp() const { return lastRealtimeData; }
    unsigned long getBalanceDataTimestamp() const { return lastBalanceData; }
    void updateLastRealtimeDataTimestamp(unsigned long value) { lastRealtimeData = value; }
    void updateBalanceDataTimestamp(unsigned long value) { lastBalanceData = value; }

  private:
    unsigned long lastRealtimeData = 0;
    unsigned long lastBalanceData = 0;

    std::string handleFirmwareVersion(const uint8_t *data, size_t length, VescData *vescData);
    std::string handleDecodedBalance(const uint8_t *data, size_t length, VescData *vescData);
    std::string handleCustomAppData(const uint8_t *data, size_t length, VescData *vescData);
    std::string handleGetValuesSelective(const uint8_t *data, size_t length, VescData *vescData, double batteryDrift);
    std::string handleGetValuesSetupSelective(const uint8_t *data, size_t length, VescData *vescData, double batteryDrift);
    std::string handleGetValues(const uint8_t *data, size_t length, VescData *vescData, double batteryDrift);

    static uint8_t readUInt8(const uint8_t *data, size_t length, size_t offset);
    static int8_t readInt8(const uint8_t *data, size_t length, size_t offset);
    static int16_t readInt16(const uint8_t *data, size_t length, size_t offset);
    static int32_t readInt32(const uint8_t *data, size_t length, size_t offset);
    static float readFloat(const uint8_t *data, size_t length, size_t offset);
    static std::string readString(const uint8_t *data, size_t length, size_t offset, size_t charCount);
};

#endif //__VESCCOMMANDPARSER_H__
