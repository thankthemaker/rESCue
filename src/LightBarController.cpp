#include "LightBarController.h"

#ifdef LIGHT_BAR_ENABLED
Adafruit_NeoPixel lightPixels;
#endif

int pixel_count = 0;
int min_voltage = 0;
int max_voltage = 0;
int voltage_range =  0;

AdcState lastAdcState = AdcState::ADC_NONE;
unsigned long lastAdcStateChange = 0;
LightBarController *LightBarController::instance = 0;

LightBarController *LightBarController::getInstance() {
    if (instance == 0) {
        instance = new LightBarController();
        pixel_count = AppConfiguration::getInstance()->config.numberPixelBatMon;
        min_voltage = AppConfiguration::getInstance()->config.minBatteryVoltage * 100;
        max_voltage = AppConfiguration::getInstance()->config.maxBatteryVoltage * 100;
        voltage_range =  max_voltage - min_voltage;
        lightPixels.begin(); // This initializes the NeoPixel library.
    }
    return instance;
}

// updates the light bar, depending on the LED count
void LightBarController::updateLightBar(float voltage, AdcState adcState, double erpm) {
    /*
    if(abs(erpm) > 10000) {
        for(int i=0; i<pixel_count; i++) 
          lightPixels.setPixelColor(i, 0, 0, 0);
        lightPixels.show();
        return;
    }
    */
    int used = max_voltage - voltage * 100; // calculate how much the voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    float diffPerPixel =
            voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    float count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine

    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

    if (Logger::getLogLevel() == Logger::VERBOSE) {
        Logger::verbose(LOG_TAG_LIGHTBAR, String("used=" + String(used) + ", value=" + String(value)).c_str());
        Logger::verbose(LOG_TAG_LIGHTBAR,
                        String("count=" + String(count) + ", diffPerPixel=" + String(diffPerPixel)).c_str());
        Logger::verbose(LOG_TAG_LIGHTBAR,
                        String("whole=" + String(whole) + ", remainder=" + String(remainder)).c_str());
    }

    if (adcState != lastAdcState) {
        for (int i = 0; i < pixel_count; i++) {
            lightPixels.setPixelColor(i, 0, 0, 0);
            switch (adcState) {
                case ADC_NONE:
                    lightPixels.setPixelColor(i, 153, 0, 153); // full purple
                    break;
                case ADC_HALF_ADC1:
                    if(i>(pixel_count/2)) {
                        lightPixels.setPixelColor(i, 153, 0, 153); // half purple
                    }
                    break;
                case ADC_HALF_ADC2:
                    if(i<(pixel_count/2)) {
                        lightPixels.setPixelColor(i, 153, 0, 153); // half purple
                    }
                    break;
                case ADC_FULL:
                    lightPixels.setPixelColor(i, 0, 0, 153); // full blue
                    break;
            }
        }
        lastAdcStateChange = millis();
    } else if (adcState == lastAdcState && millis() - lastAdcStateChange >= 2000) {
        // update every pixel individually
        for (int i = 0; i < pixel_count; i++) {
            if (i == whole) {
                // the last pixel, the battery voltage somewhere in the range of this pixel
                // the lower the remaining value the more the pixel goes from green to red
                int val = calcVal(remainder);
                lightPixels.setPixelColor(i, MAX_BRIGHTNESS - val, val, 0);
            }
            if (i > whole) {
                // these pixels must be turned off, we already reached a lower battery voltage
                lightPixels.setPixelColor(i, 0, 0, 0);
            }
            if (i < whole) {
                // turn on this pixel completely green, the battery voltage is still above this value
                lightPixels.setPixelColor(i, 0, MAX_BRIGHTNESS, 0);
            }
            if (value < 0) {
                // ohhh, we already hit the absolute minimum, set all pixel to full red.
                lightPixels.setPixelColor(i, MAX_BRIGHTNESS, 0, 0);
            }
        }
    }
#ifdef LIGHT_BAR_ADC_ENABLED
    lastAdcState = adcState;
#endif
    lightPixels.show();
}

// map the remaining value to a value between 0 and MAX_BRIGHTNESS
int LightBarController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}