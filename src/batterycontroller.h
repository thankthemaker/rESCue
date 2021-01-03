#ifndef BATTERY_CONTROLLER_H
#define BATTERY_CONTROLLER_H

#include "config.h"
#include "buzzer.h"

class BatteryController {
    public:
        BatteryController();
        int readVoltage();
        void checkVoltage(Buzzer *buzzer);
};

#endif