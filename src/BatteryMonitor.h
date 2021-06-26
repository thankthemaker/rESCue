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
        BatteryMonitor();
#ifdef CANBUS_ENABLED
        BatteryMonitor(CanBus::VescData *vescData);
#endif
        void init();
        float readValues();
        void checkValues();

    private:
#ifdef CANBUS_ENABLED
        CanBus::VescData *vescData;
#endif
        int smoothAnalogReading();
        void updateCurrentArray(double value);
        double getAverageCurrent();
        AdcState mapSwitchState(uint16_t intState, boolean isAdc1Enabled);
};

#endif //__BATTERY_CONTROLLER_H__