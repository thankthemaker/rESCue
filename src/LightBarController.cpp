#include "LightBarController.h"

Adafruit_NeoPixel lightPixels = Adafruit_NeoPixel(AppConfiguration::getInstance()->config.numberPixelBatMon,
                                                  LIGHT_BAR_PIN, NEO_GRB + NEO_KHZ800);

int pixel_count = 0;
int min_voltage = 0;
int max_voltage = 0;
int voltage_range = 0;
boolean pixelCountOdd = false;

AdcState lastAdcState = AdcState::ADC_NONE;
unsigned long lastAdcStateChange = 0;

LightBarController::LightBarController() {}

void LightBarController::init() {
    pixel_count = AppConfiguration::getInstance()->config.numberPixelBatMon;
    min_voltage = (int) AppConfiguration::getInstance()->config.minBatteryVoltage * 100;
    max_voltage = (int) AppConfiguration::getInstance()->config.maxBatteryVoltage * 100;
    pixelCountOdd = pixel_count % 2 == 1;
    uint8_t ledType;
    if(AppConfiguration::getInstance()->config.isLightBarLedTypeDifferent) {
        ledType = LedControllerFactory::getInstance()->determineLedType(true);
    } else {
        ledType = LedControllerFactory::getInstance()->determineLedType();
    }
    voltage_range = max_voltage - min_voltage;
    lightPixels.updateLength(pixel_count);
    lightPixels.updateType(ledType);
    lightPixels.begin(); // This initializes the NeoPixel library.
    for (int j = 0; j < pixel_count; j++) {
        int actualIndex = j;
        if(AppConfiguration::getInstance()->config.isLightBarReversed) {
            actualIndex = pixel_count - 1 - j;
        }
        lightPixels.setPixelColor(actualIndex, 51, 153, 255);
    }
    lightPixels.show();
}


// updates the light bar, depending on the LED count
void LightBarController::updateLightBar(double voltage, uint16_t switchstate, double adc1, double adc2, double erpm) {
    AdcState adcState = this->mapSwitchState(switchstate, adc1 > adc2);
    if (abs(erpm) > AppConfiguration::getInstance()->config.lightbarTurnOffErpm) {
        for (int i = 0; i < pixel_count; i++)
            lightPixels.setPixelColor(i, 0, 0, 0);
        lightPixels.show();
        return;
    }
    int used = max_voltage - voltage * 100; // calculate how much the voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    double diffPerPixel =
            voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    double count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine

    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

    if (esp_log_level_get(LOG_TAG_LIGHTBAR) >= ESP_LOG_DEBUG) {
        ESP_LOGD(LOG_TAG_LIGHTBAR, "used=%d, value=%d", used, value);
        ESP_LOGD(LOG_TAG_LIGHTBAR, "count=%f, diffPerPixel=%f", count, diffPerPixel);
        ESP_LOGD(LOG_TAG_LIGHTBAR, "whole=%d, remainder=%d", whole, remainder);
    }

    if (adcState != lastAdcState) {
        for (int i = 0; i < pixel_count; i++) {
            int actualIndex = i;
            if(AppConfiguration::getInstance()->config.isLightBarReversed) {
                actualIndex = pixel_count - 1 - i;
            }
            lightPixels.setPixelColor(actualIndex, 0, 0, 0);
            switch (adcState) {
                case ADC_NONE:
                    lightPixels.setPixelColor(actualIndex, 153, 0, 153); // full purple
                    break;
                case ADC_HALF_ADC1:
                    if ((pixelCountOdd && i > (pixel_count / 2)) || (!pixelCountOdd && i >= (pixel_count / 2))) {
                        lightPixels.setPixelColor(i, 153, 0, 153); // half purple
                    }
                    break;
                case ADC_HALF_ADC2:
                    if (i < (pixel_count / 2)) {
                        lightPixels.setPixelColor(actualIndex, 153, 0, 153); // half purple
                    }
                    break;
                case ADC_FULL:
                    lightPixels.setPixelColor(actualIndex, 0, 0, 153); // full blue
                    break;
            }
        }
        lastAdcStateChange = millis();
    } else if (adcState == lastAdcState && millis() - lastAdcStateChange >= 2000) {
        // update every pixel individually
        for (int i = 0; i < pixel_count; i++) {
            int actualIndex = i;
            if(AppConfiguration::getInstance()->config.isLightBarReversed) {
                actualIndex = pixel_count - 1 - i;
            }
            if (i == whole) {
                // the last pixel, the battery voltage somewhere in the range of this pixel
                // the lower the remaining value the more the pixel goes from green to red
                int val = calcVal(remainder);
                lightPixels.setPixelColor(i, AppConfiguration::getInstance()->config.lightbarMaxBrightness - val, val, 0);
            }
            if (i > whole) {
                // these pixels must be turned off, we already reached a lower battery voltage
                lightPixels.setPixelColor(actualIndex, 0, 0, 0);
            }
            if (i < whole) {
                // turn on this pixel completely green, the battery voltage is still above this value
                lightPixels.setPixelColor(actualIndex, 0, AppConfiguration::getInstance()->config.lightbarMaxBrightness, 0);
            }
            if (value < 0) {
                // ohhh, we already hit the absolute minimum, set all pixel to full red.
                lightPixels.setPixelColor(actualIndex, AppConfiguration::getInstance()->config.lightbarMaxBrightness, 0, 0);
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
    return map(value, 0, 100, 0, AppConfiguration::getInstance()->config.lightbarMaxBrightness);
}

AdcState LightBarController::mapSwitchState(uint16_t intState, boolean isAdc1Enabled) {
    //Serial.printf("Map Switchstate: %d %d\n", intState, isAdc1Enabled);
    switch (intState) {
        case 0:
            return AdcState::ADC_NONE;
        case 1:
            return isAdc1Enabled ? AdcState::ADC_HALF_ADC1 : AdcState::ADC_HALF_ADC2;
        case 2:
            return AdcState::ADC_FULL;
        default:
            ESP_LOGE(LOG_TAG_LIGHTBAR, "Unknown switch state");
    }
    return AdcState::ADC_NONE;
}