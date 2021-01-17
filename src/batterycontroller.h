#ifndef __BATTERY_CONTROLLER_H__
#define __BATTERY_CONTROLLER_H__

#include "config.h"
#include "Buzzer.h"
#include "CanBus.h"

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
        BatteryController(CanBus::VescData &vescData);
        void init();
        float readVoltage();
        void checkVoltage(Buzzer *buzzer);
    private:
        CanBus::VescData vescData;

        void updateBatteryBar(float voltage);
        int calcVal(int value);
        int smoothAnalogReading();
};

#endif //__BATTERY_CONTROLLER_H__