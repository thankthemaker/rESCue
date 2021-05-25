#include "LightBarController.h"

#ifdef LIGHT_BAR_ENABLED
 Adafruit_NeoPixel lightPixels = Adafruit_NeoPixel(AppConfiguration::getInstance()->config.numberPixelBatMon, LIGHT_BAR_PIN, NEO_GRB + NEO_KHZ800);
#endif

const int pixel_count   = AppConfiguration::getInstance()->config.numberPixelBatMon;
const int min_voltage   = MIN_BATTARY_VOLTAGE;
const int max_voltage   = MAX_BATTARY_VOLTAGE;
const int warn_voltage  = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
const int voltage_range = MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE;
const double max_current= MAX_AVG_CURRENT;

AdcState lastAdcState = AdcState::ACD_NONE;

LightBarController* LightBarController::instance = 0;

LightBarController* LightBarController::getInstance() {
    if (instance == 0){
        instance = new LightBarController();
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
    float diffPerPixel = voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    float count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine
    
    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

    if(Logger::getLogLevel() == Logger::VERBOSE) {
      Logger::verbose(LOG_TAG_LIGHTBAR, String("used=" + String(used) + ", value=" + String(value)).c_str());
      Logger::verbose(LOG_TAG_LIGHTBAR, String("count=" + String(count) + ", diffPerPixel=" + String(diffPerPixel)).c_str());
      Logger::verbose(LOG_TAG_LIGHTBAR, String("whole=" + String(whole) + ", remainder=" + String(remainder)).c_str());
    }

    // update every pixel individually
    for(int i=0; i<pixel_count; i++) {
        if(i==whole) {
            // the last pixel, the battery voltage somewhere in the range of this pixel
            // the lower the remaining value the more the pixel goes from green to red
            int val = calcVal(remainder);
            lightPixels.setPixelColor(i, MAX_BRIGHTNESS-val, val, 0);
        }
        if(i>whole) {
            // these pixels must be turned off, we already reached a lower battery voltage 
            lightPixels.setPixelColor(i, 0, 0, 0);
        } 
        if(i<whole) {
            // turn on this pixel completely green, the battery voltage is still above this value
            lightPixels.setPixelColor(i, 0, MAX_BRIGHTNESS, 0);
        }
        if(value < 0) {
            // ohhh, we already hit the absolute minimum, set all pixel to full red.
            lightPixels.setPixelColor(i, MAX_BRIGHTNESS, 0, 0);
        }
    }
    lightPixels.show();

#ifdef LIGHT_BAR_ADC_ENABLED
  lastAdcState = adcState;
#endif
}

// map the remaining value to a value between 0 and MAX_BRIGHTNESS
int LightBarController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}