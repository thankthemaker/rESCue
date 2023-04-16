#include "BatteryMonitor.h"

#define LOG_TAG_BATMON "BatteryMonitor"

const int numCurReadings = 50;

unsigned long lastCheck = 0;
unsigned long  lastBatWarn = 0;
unsigned long  lastCurWarn = 0;
int total = 0; // the running total
double currentReadings[numCurReadings];

BatteryMonitor::BatteryMonitor(VescData *vescData) {
    this->vescData = vescData;
}

void BatteryMonitor::init() {
    Logger::notice(LOG_TAG_BATMON, "initializing...");
    this->min_voltage = (int) AppConfiguration::getInstance()->config.minBatteryVoltage * 100;
    this->max_voltage = (int) AppConfiguration::getInstance()->config.maxBatteryVoltage * 100;
    this->warn_voltage = (int) AppConfiguration::getInstance()->config.lowBatteryVoltage * 100;
    this->max_current = (int) AppConfiguration::getInstance()->config.maxAverageCurrent;
}

// Read the voltage from the voltage divider and update the battery bar if connected
double BatteryMonitor::readValues() {
    auto voltage = (double) vescData->inputVoltage;
    auto current = (double) abs(vescData->current);
    updateCurrentArray(current);

    if (Logger::getLogLevel() == Logger::VERBOSE) {
        Logger::verbose(LOG_TAG_BATMON, String("Voltage: " + String(voltage) + "V").c_str());
    }
    return voltage;
}

// check the voltage against the configured min and max values
void BatteryMonitor::checkValues() {
    if (millis() - lastCheck < 300) {
        // only check every 300ms
        return;
    }
    lastCheck = millis();

    int voltage = (int) readValues();
    // check if voltage reading is valid, otherwise skip
    if (voltage <= 1) {
        return;
    } else {
        voltage = voltage * 100;
    }

    // check if voltage is below absolute minimum or above absolute maximum (regen)
    if (voltage < min_voltage || voltage > max_voltage) {
        Serial.printf("Voltages: %d %d, %d", min_voltage, voltage, max_voltage);
        Logger::warning(LOG_TAG_BATMON, "ALARM: Battery voltage out of range");
        Buzzer::alarm();  // play an anoying alarm tone
        return;
    }

    // check if the voltage is close to the minimum
    if (voltage < warn_voltage) {
        if (millis() - lastBatWarn > 5000) {
            Logger::warning(LOG_TAG_BATMON, "WARN: Battery voltage out of range");
            Buzzer::warning(); // play a warn tonen every 5 seconds
            lastBatWarn = millis();
        }
    }

    // check if the average current is higher max
    if (getAverageCurrent() > max_current) {
        if (millis() - lastCurWarn > 5000) {
            Logger::warning(LOG_TAG_BATMON, "WARN: Average current too high");
            Buzzer::warning(); // play a warn tonen every 5 seconds
            lastCurWarn = millis();
        }
    }
}
void BatteryMonitor::updateCurrentArray(double value) {
    for (int i = numCurReadings; i > 0; i--) {
        currentReadings[i] = currentReadings[i - 1];
    }
    currentReadings[0] = value;
}
double BatteryMonitor::getAverageCurrent() {
    double avg = 0;
    for (double currentReading : currentReadings) {
        avg += currentReading;
    }
    return avg / numCurReadings;
}