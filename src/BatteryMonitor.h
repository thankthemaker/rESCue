#ifndef __BATTERY_CONTROLLER_H__
#define __BATTERY_CONTROLLER_H__

#include <Logger.h>
#include "config.h"
#include "Buzzer.h"
#include "CanBus.h"

class BatteryMonitor {
    public:
        explicit BatteryMonitor(VescData *vescData);
        void init();
        double readValues();
        void checkValues();

    private:
        int min_voltage = 0;
        int max_voltage = 0;
        int warn_voltage = 0;
        double max_current = 0;
        VescData *vescData;
        static void updateCurrentArray(double value);
        static double getAverageCurrent();
};

#endif //__BATTERY_CONTROLLER_H__