#ifndef __LIGHTBAR_CONTROLLER_H__
#define __LIGHTBAR_CONTROLLER_H__

#include <Arduino.h>
#include <Logger.h>
#include "config.h"
#include "AppConfiguration.h"

#define LOG_TAG_LIGHTBAR "LightBar"

#ifdef LIGHT_BAR_ENABLED
 #include <Adafruit_NeoPixel.h>
 #ifndef LIGHT_BAR_PIN
  #define LIGHT_BAR_PIN 5 // default PIN
 #endif //LIGHT_BAR_PIN
 extern Adafruit_NeoPixel lightPixels;
#endif //LIGHT_BAR_ENABLED

enum  ErrorCode { ERR_NONE };
enum  AdcState { ACD_NONE, ADC_HALF, ADC_FULL };

class LightBarController {
    public:
        static LightBarController* getInstance();
        void updateLightBar(float voltage, AdcState adcState, double erpm);

    private:
        LightBarController() {}
        static LightBarController *instance; 
        int calcVal(int value);
};

#endif