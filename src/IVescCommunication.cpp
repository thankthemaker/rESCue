#include "IVescCommunication.h"
#include "CanBus.h"
#include "Uart.h"

#include <string>

VescCommunicationFactory *VescCommunicationFactory::instance = nullptr;

VescCommunicationFactory *VescCommunicationFactory::getInstance() {
    if (instance == nullptr) {
        instance = new VescCommunicationFactory();
    }
    return instance;
}

IVescCommunication *VescCommunicationFactory::createVescCommunication(VescData *vescData) {
#if defined(CANBUS_ENABLED)
    return new CanBus(vescData);
#else
    // UART is the fallback for every build that does not enable CANBUS.
    return new Uart(vescData);
#endif
}

VescCommunicationBase::VescCommunicationBase(VescData *vescData)
        : vescData(vescData), vescCommandParser(new VescCommandParser()) {}

void VescCommunicationBase::loop() {
    const unsigned long now = millis();

    if (initialized) {
        if (!requestSlotBusy() &&
            now >= vescCommandParser->getLastRealtimeDataTimestamp() &&
            now - vescCommandParser->getLastRealtimeDataTimestamp() > (unsigned long) interval) {
            if (!sendRealtimeDataRequest()) {
                vescCommandParser->updateLastRealtimeDataTimestamp(now + 500);
                vescData->connected = false;
            }
        }

        if (!requestSlotBusy() &&
            now >= vescCommandParser->getBalanceDataTimestamp() &&
            now - vescCommandParser->getBalanceDataTimestamp() > (unsigned long) interval) {
            if (!sendFloatPackageDataRequest()) {
                vescCommandParser->updateBalanceDataTimestamp(now + 500);
                vescData->connected = false;
            }
        }
    } else if (initRetryCounter > 0 && now >= lastRetry && now - lastRetry > 500) {
        sendFirmwareVersionRequest();
        initRetryCounter--;
        lastRetry = now;
        if (initRetryCounter == 0) {
            ESP_LOGE(LOG_TAG_VESC_COMMUNICATION, "VESC initialization failed, will keep retrying");
            initRetryCounter = 1;
            lastRetry = now + 5000;
        }
    }

    readFromVesc();
    afterRead();
}

namespace {

template<typename T>
void appendField(std::string &out, const char *name, const T &value) {
    out += name;
    out += '=';
    out += std::to_string(value);
    out += ", ";
}

void appendField(std::string &out, const char *name, const std::string &value) {
    out += name;
    out += '=';
    out += value;
    out += ", ";
}

} // namespace

void VescCommunicationBase::dumpVescValues() {
    if (esp_log_level_get(LOG_TAG_VESC_COMMUNICATION) < ESP_LOG_DEBUG || millis() - lastDump < 1000) {
        return;
    }

    std::string bufferString;
    bufferString.reserve(512);
    appendField(bufferString, "name", vescData->name);
    appendField(bufferString, "dutycycle", vescData->dutyCycle);
    appendField(bufferString, "erpm", vescData->erpm);
    appendField(bufferString, "current", vescData->current);
    appendField(bufferString, "ampHours", vescData->ampHours);
    appendField(bufferString, "ampHoursCharged", vescData->ampHoursCharged);
    appendField(bufferString, "wattHours", vescData->wattHours);
    appendField(bufferString, "wattHoursCharged", vescData->wattHoursCharged);
    appendField(bufferString, "mosfetTemp", vescData->mosfetTemp);
    appendField(bufferString, "motorTemp", vescData->motorTemp);
    appendField(bufferString, "inputVoltage", vescData->inputVoltage);
    appendField(bufferString, "tachometer", vescData->tachometer);
    appendField(bufferString, "pidOutput", vescData->pidOutput);
    appendField(bufferString, "pitch", vescData->pitch);
    appendField(bufferString, "roll", vescData->roll);
    ESP_LOGD(LOG_TAG_VESC_COMMUNICATION, "%s", bufferString.c_str());

    lastDump = millis();
}
