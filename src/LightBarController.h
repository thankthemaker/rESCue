#ifndef __LIGHTBAR_CONTROLLER_H__
#define __LIGHTBAR_CONTROLLER_H__

#include <Arduino.h>
#include <Logger.h>
#include "config.h"
#include "AppConfiguration.h"
#include "ILedController.h"

#define LOG_TAG_LIGHTBAR "LightBar"


#include <Adafruit_NeoPixel.h>

#ifndef LIGHT_BAR_PIN
#define LIGHT_BAR_PIN 2 // default PIN
#endif //LIGHT_BAR_PIN
extern Adafruit_NeoPixel lightPixels;

enum ErrorCode {
    ERR_NONE
};
enum AdcState {
    ADC_NONE, ADC_HALF_ADC1, ADC_HALF_ADC2, ADC_FULL
};

class LightBarController {
public:
    LightBarController();
    void updateLightBar(double voltage, uint16_t switchstate, double adc1, double adc2, double erpm);

private:
    static AdcState mapSwitchState(uint16_t intState, boolean isAdc1Enabled);

    static LightBarController *instance;

    static int calcVal(int value);
};

#endif