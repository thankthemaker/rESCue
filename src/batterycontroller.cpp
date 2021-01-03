#include "BatteryController.h"

int min_voltage   = MIN_BATTARY_VOLTAGE;
int max_voltage   = MAX_BATTARY_VOLTAGE;
int warn_voltage  = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
int voltage_range = MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE;
int pixel_count   = BATTERY_BAR_NUMPIXELS;
int lastWarn      = 0;

#ifdef BATTERY_BAR
 Adafruit_NeoPixel batPixels = Adafruit_NeoPixel(BATTERY_BAR_NUMPIXELS, BATTERY_BAR_PIN, NEO_GRB + NEO_KHZ800);
#endif

BatteryController::BatteryController() {}

void BatteryController::init() {
#if DEBUG > 0
  Serial.println("Initializing BatteryController");
#endif
  batPixels.begin(); // This initializes the NeoPixel library.
}

int BatteryController::readVoltage() {
    int sensorValue = analogRead(BATTERY_PIN);
    float voltage = sensorValue *  VOLTAGE_DIVIDER_CONSTANT;
#ifdef BATTERY_BAR
    updateBatteryBar(voltage);
#endif
#if DEBUG > 0
    // print out the value you read:
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
#endif
    return voltage;
}

void BatteryController::checkVoltage(Buzzer *buzzer) {
    int voltage = readVoltage();
    if(voltage < min_voltage || voltage > max_voltage) {
        buzzer->alarm();
        return;
    } 
    if(voltage < warn_voltage) {
        if(millis() - lastWarn > 5000) {
            buzzer->beep(3);
        }
    }
}

void BatteryController::updateBatteryBar(float voltage) {
    int used = max_voltage - voltage*100; // calculate how much voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    float diffPerPixel = voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    float count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine
    
    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

#if DEBUG > 1
    Serial.println("used=" + String(used) + ", value=" + String(value));
    Serial.println("count=" + String(count) + ", diffPerPixel=" + String(diffPerPixel));
    Serial.println("whole=" + String(whole) + ", remainder=" + String(remainder));
#endif

    for(int i=0; i<pixel_count; i++) {
        if(i==whole) {
            int val = calcVal(remainder);
            batPixels.setPixelColor(i, MAX_BRIGHTNESS-val, val, 0);
        } else if(i>whole) {
            batPixels.setPixelColor(i, 0, 0, 0);
        } else {
            batPixels.setPixelColor(i, 0, MAX_BRIGHTNESS, 0);
        }
    }
    batPixels.show();
}

int BatteryController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}