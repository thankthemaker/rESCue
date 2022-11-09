#ifndef __BATTERY_CONTROLLER_H__
#define __BATTERY_CONTROLLER_H__

#include <Logger.h>
#include "config.h"
#include "Buzzer.h"
#ifdef CANBUS_ENABLED
 #include "CanBus.h"
#endif
#ifdef LIGHT_BAR_ENABLED
 #include "LightBarController.h"
#endif

class BatteryMonitor {
    public:
        BatteryMonitor(VescData *vescData);
        void init();
        float readValues();
        void checkValues();

    private:
        int min_voltage = 0;
        int max_voltage = 0;
        int warn_voltage = 0;
        double max_current = 0;
        VescData *vescData;
        int smoothAnalogReading();
        void updateCurrentArray(double value);
        double getAverageCurrent();
        AdcState mapSwitchState(uint16_t intState, boolean isAdc1Enabled);
};

#endif //__BATTERY_CONTROLLER_H__