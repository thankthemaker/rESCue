//
// Created by David Anthony Gey on 14.10.22.
//

#ifndef RESCUE_VESCDATA_H
#define RESCUE_VESCDATA_H

#include <cstdint>

struct VescData {
    uint8_t majorVersion;
    uint8_t minorVersion;
    std::string name;
    std::string uuid;

    double dutyCycle = 0.0;
    double erpm = 0.0;
    double current = 0.0;
    double ampHours = 0.0;
    double ampHoursCharged= 0.0;
    double wattHours = 0.0;
    double wattHoursCharged = 0.0;
    double mosfetTemp = 0.0;
    double motorTemp = 0.0;
    double totalCurrentIn = 0.0;
    double pidPosition = 0.0;
    double inputVoltage = 0.0;
    double tachometer = 0.0;
    double tachometerAbsolut = 0.0;

    double pidOutput = 0.0;
    double pitch = 0.0;
    double roll = 0.0;
    uint32_t loopTime = 0;
    double motorCurrent = 0.0;
    double motorPosition = 0.0;
    uint16_t balanceState = 0;
    uint16_t switchState = 0;
    double adc1 = 0.0;
    double adc2 = 0.0;
    uint8_t fault = 0;
};

#endif //RESCUE_VESCDATA_H
