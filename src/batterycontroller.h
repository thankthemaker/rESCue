#ifndef BATTERY_CONTROLLER_H
#define BATTERY_CONTROLLER_H

#include "config.h"
#include "buzzer.h"

#ifdef BATTERY_BAR
 #include <Adafruit_NeoPixel.h>
 #ifndef BATTERY_BAR_PIN
  #define BATTERY_BAR_PIN 5 // default PIN
 #endif
 extern Adafruit_NeoPixel batPixels;
#endif


class BatteryController {
    public:
        BatteryController();
        void init();
        int readVoltage();
        void checkVoltage(Buzzer *buzzer);
    //private:
        void updateBatteryBar(float voltage);
        int calcVal(int value);
};

#endif