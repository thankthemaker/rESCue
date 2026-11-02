#include "VescCommandParser.h"
#include "buffer.h"

namespace {
constexpr uint8_t COMM_FW_VERSION = 0x00;
constexpr uint8_t COMM_GET_VALUES = 0x04;
constexpr uint8_t COMM_CUSTOM_APP_DATA = 0x24;
constexpr uint8_t COMM_GET_VALUES_SELECTIVE = 0x32;
constexpr uint8_t COMM_GET_VALUES_SETUP_SELECTIVE = 0x33;
constexpr uint8_t COMM_GET_DECODED_BALANCE = 0x4F;

// marker of the float package inside COMM_CUSTOM_APP_DATA
constexpr uint8_t FLOAT_PACKAGE = 101;
constexpr uint8_t FLOAT_COMMAND_GET_RTDATA = 0x1;
} // namespace

std::string VescCommandParser::parseCommand(const uint8_t *data, size_t length, VescData *vescData, double batteryDrift,
                                            unsigned long now) {
    if (data == nullptr || length == 0) {
        return "";
    }

    switch (data[0]) {
        case COMM_FW_VERSION:
            return handleFirmwareVersion(data, length, vescData);
        case COMM_GET_DECODED_BALANCE:
            lastBalanceData = now;
            return handleDecodedBalance(data, length, vescData);
        case COMM_CUSTOM_APP_DATA:
            lastBalanceData = now;
            return handleCustomAppData(data, length, vescData);
        case COMM_GET_VALUES_SELECTIVE:
            lastRealtimeData = now;
            return handleGetValuesSelective(data, length, vescData, batteryDrift);
        case COMM_GET_VALUES_SETUP_SELECTIVE:
            lastRealtimeData = now;
            return handleGetValuesSetupSelective(data, length, vescData, batteryDrift);
        case COMM_GET_VALUES:
            lastRealtimeData = now;
            return handleGetValues(data, length, vescData, batteryDrift);
        default:
            return "";
    }
}

std::string VescCommandParser::handleFirmwareVersion(const uint8_t *data, size_t length, VescData *vescData) {
    vescData->majorVersion = readUInt8(data, length, 1);
    vescData->minorVersion = readUInt8(data, length, 2);
    vescData->name = readString(data, length, 3, 12);
    return "COMM_FW_VERSION";
}

std::string VescCommandParser::handleDecodedBalance(const uint8_t *data, size_t length, VescData *vescData) {
    const size_t offset = 1;
    vescData->pidOutput = readInt32(data, length, 0 + offset) / 1000000.0;
    vescData->pitch = readInt32(data, length, 4 + offset) / 1000000.0;
    vescData->roll = readInt32(data, length, 8 + offset) / 1000000.0;
    vescData->loopTime = readInt32(data, length, 12 + offset);
    vescData->motorCurrent = readInt32(data, length, 16 + offset) / 1000000.0;
    vescData->motorPosition = readInt32(data, length, 20 + offset) / 1000000.0;
    vescData->balanceState = readInt16(data, length, 24 + offset);
    vescData->switchState = readInt16(data, length, 26 + offset);
    vescData->adc1 = readInt32(data, length, 28 + offset) / 1000000.0;
    vescData->adc2 = readInt32(data, length, 32 + offset) / 1000000.0;
    return "COMM_GET_DECODED_BALANCE";
}

std::string VescCommandParser::handleCustomAppData(const uint8_t *data, size_t length, VescData *vescData) {
    if (readUInt8(data, length, 1) == FLOAT_PACKAGE &&
        readUInt8(data, length, 2) == FLOAT_COMMAND_GET_RTDATA) {
        const size_t offset = 3;
        vescData->pidOutput = readFloat(data, length, 0 + offset);
        vescData->pitch = readFloat(data, length, 4 + offset);
        vescData->roll = readFloat(data, length, 8 + offset);
        vescData->balanceState = readUInt8(data, length, 12 + offset);
        const uint16_t switchState = readUInt8(data, length, 13 + offset);
        vescData->adc1 = readFloat(data, length, 14 + offset);
        vescData->adc2 = readFloat(data, length, 18 + offset);

        switch (switchState) {
            case 0:
                vescData->switchState = 0;
                break;
            case 1:
                vescData->switchState = (vescData->adc1 > vescData->adc2) ? 1 : 2;
                break;
            case 2:
                vescData->switchState = 3;
                break;
            default:
                break;
        }
    }
    return "COMM_CUSTOM_APP_DATA";
}

std::string VescCommandParser::handleGetValuesSelective(const uint8_t *data, size_t length, VescData *vescData,
                                                        double batteryDrift) {
    const size_t offset = 1;
    vescData->mosfetTemp = readInt16(data, length, 4 + offset) / 10.0;
    vescData->motorTemp = readInt16(data, length, 6 + offset) / 10.0;
    vescData->dutyCycle = readInt16(data, length, 8 + offset) / 1000.0;
    vescData->erpm = readInt32(data, length, 10 + offset);
    vescData->inputVoltage = readInt16(data, length, 14 + offset) / 10.0 + batteryDrift;
    vescData->tachometer = readInt32(data, length, 16 + offset);
    vescData->tachometerAbsolut = readInt32(data, length, 20 + offset);
    vescData->fault = readUInt8(data, length, 24 + offset);
    return "COMM_GET_VALUES_SELECTIVE";
}

std::string VescCommandParser::handleGetValuesSetupSelective(const uint8_t *data, size_t length, VescData *vescData,
                                                             double batteryDrift) {
    const size_t offset = 1;
    size_t startbyte = 0;
    const uint32_t bitmask = (uint32_t) readInt32(data, length, 0 + offset);
    startbyte += 4;

    if (bitmask & ((uint32_t) 1 << 0)) {
        vescData->mosfetTemp = readInt16(data, length, startbyte + offset) / 10.0;
        startbyte += 2;
    }
    if (bitmask & ((uint32_t) 1 << 1)) {
        vescData->motorTemp = readInt16(data, length, startbyte + offset) / 10.0;
        startbyte += 2;
    }
    if (bitmask & ((uint32_t) 1 << 2)) {  // current in
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 3)) {  // current in_total
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 4)) {
        vescData->dutyCycle = readInt16(data, length, startbyte + offset) / 1000.0;
        startbyte += 2;
    }
    if (bitmask & ((uint32_t) 1 << 5)) {
        vescData->erpm = readInt32(data, length, startbyte + offset);
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 6)) {  // speed
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 7)) {
        vescData->inputVoltage = readInt16(data, length, startbyte + offset) / 10.0 + batteryDrift;
        startbyte += 2;
    }
    if (bitmask & ((uint32_t) 1 << 8)) {  // battery level
        startbyte += 2;
    }
    if (bitmask & ((uint32_t) 1 << 9)) {  // amphours consumed
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 10)) {  // amphours charged
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 11)) {  // watthours consumed
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 12)) {  // watthours charged
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 13)) {
        vescData->tachometer = readInt32(data, length, startbyte + offset);
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 14)) {
        vescData->tachometerAbsolut = readInt32(data, length, startbyte + offset);
        startbyte += 4;
    }
    if (bitmask & ((uint32_t) 1 << 16)) {
        vescData->fault = readUInt8(data, length, startbyte + offset);
        startbyte += 1;
    }
    return "COMM_GET_VALUES_SETUP_SELECTIVE";
}

std::string VescCommandParser::handleGetValues(const uint8_t *data, size_t length, VescData *vescData,
                                               double batteryDrift) {
    const size_t offset = 1;
    vescData->mosfetTemp = readInt16(data, length, 0 + offset) / 10.0;
    vescData->motorTemp = readInt16(data, length, 2 + offset) / 10.0;
    vescData->motorCurrent = readInt32(data, length, 4 + offset) / 100.0;
    vescData->current = readInt32(data, length, 8 + offset) / 100.0;
    vescData->dutyCycle = readInt16(data, length, 20 + offset) / 1000.0;
    vescData->erpm = readInt32(data, length, 22 + offset);
    vescData->inputVoltage = readInt16(data, length, 26 + offset) / 10.0 + batteryDrift;
    vescData->ampHours = readInt32(data, length, 28 + offset) / 10000.0;
    vescData->ampHoursCharged = readInt32(data, length, 32 + offset) / 10000.0;
    vescData->wattHoursCharged = readInt32(data, length, 40 + offset) / 10000.0;
    vescData->tachometer = readInt32(data, length, 44 + offset);
    vescData->wattHours = readInt32(data, length, 46 + offset) / 10000.0;
    vescData->fault = readUInt8(data, length, 52 + offset);
    vescData->tachometerAbsolut = readInt32(data, length, 58 + offset);
    return "COMM_GET_VALUES";
}

uint8_t VescCommandParser::readUInt8(const uint8_t *data, size_t length, size_t offset) {
    return offset < length ? data[offset] : 0;
}

int8_t VescCommandParser::readInt8(const uint8_t *data, size_t length, size_t offset) {
    return (int8_t) readUInt8(data, length, offset);
}

int16_t VescCommandParser::readInt16(const uint8_t *data, size_t length, size_t offset) {
    if (offset + 2 > length) {
        return 0;
    }
    return (int16_t) (((uint16_t) data[offset] << 8) | (uint16_t) data[offset + 1]);
}

int32_t VescCommandParser::readInt32(const uint8_t *data, size_t length, size_t offset) {
    if (offset + 4 > length) {
        return 0;
    }
    return (int32_t) (((uint32_t) data[offset] << 24) |
                      ((uint32_t) data[offset + 1] << 16) |
                      ((uint32_t) data[offset + 2] << 8) |
                      (uint32_t) data[offset + 3]);
}

float VescCommandParser::readFloat(const uint8_t *data, size_t length, size_t offset) {
    if (offset + 4 > length) {
        return 0.0f;
    }
    int32_t index = (int32_t) offset;
    return buffer_get_float32_auto(data, &index);
}

std::string VescCommandParser::readString(const uint8_t *data, size_t length, size_t offset, size_t charCount) {
    std::string value;
    for (size_t i = offset; i < offset + charCount && i < length; i++) {
        value += (char) data[i];
    }
    return value;
}
