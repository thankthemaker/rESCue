#ifndef __BATTERY_CONTROLLER_H__
#define __BATTERY_CONTROLLER_H__

#include "config.h"
#include "Buzzer.h"
#ifdef CANBUS_ENABLED
 #include "CanBus.h"
#endif

#ifdef BATTERY_BAR
 #include <Adafruit_NeoPixel.h>
 #ifndef BATTERY_BAR_PIN
  #define BATTERY_BAR_PIN 5 // default PIN
 #endif //BATTERY_BAR_PIN
 extern Adafruit_NeoPixel batPixels;
#endif //BATTERY_BAR


class BatteryController {
    public:
        BatteryController();
#ifdef CANBUS_ENABLED
        BatteryController(CanBus::VescData *vescData);
#endif
        void init();
        float readVoltage();
        void checkVoltage(Buzzer *buzzer);
    private:
#ifdef CANBUS_ENABLED
        CanBus::VescData *vescData;
#endif
        void updateBatteryBar(float voltage);
        int calcVal(int value);
        int smoothAnalogReading();
};

#endif //__BATTERY_CONTROLLER_H__