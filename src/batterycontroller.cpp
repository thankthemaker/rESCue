#include "BatteryController.h"

const int numReadings   = 10;
const int min_voltage   = MIN_BATTARY_VOLTAGE;
const int max_voltage   = MAX_BATTARY_VOLTAGE;
const int warn_voltage  = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
const int voltage_range = MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE;
const int pixel_count   = BATTERY_BAR_NUMPIXELS;
int lastCheck           = 0;
int lastWarn            = 0;
int readIndex           = 0; // the index of the current reading
int total               = 0; // the running total
int average             = 0; // the average
int readings[numReadings];   // the readings from the analog input

#ifdef BATTERY_BAR
 Adafruit_NeoPixel batPixels = Adafruit_NeoPixel(BATTERY_BAR_NUMPIXELS, BATTERY_BAR_PIN, NEO_GRB + NEO_KHZ800);
#endif

BatteryController::BatteryController() {}

void BatteryController::init() {
#if DEBUG > 0
  Serial.println("Initializing BatteryController");
#endif
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  batPixels.begin(); // This initializes the NeoPixel library.
}

float BatteryController::readVoltage() {
    int adc = smoothAnalogReading();
    float sensorValue = ( adc * 3.3 ) / (4095);
    float voltage = sensorValue *  VOLTAGE_DIVIDER_CONSTANT;
#ifdef BATTERY_BAR
    updateBatteryBar(voltage);
#endif
#if DEBUG > 1
    Serial.print("ADC: ");
    Serial.println(adc);
    Serial.print("Sensorvalue: ");
    Serial.print(sensorValue);
    Serial.println(" V");
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
#endif
    return voltage;
}

void BatteryController::checkVoltage(Buzzer *buzzer) {
    if(millis() - lastCheck < 300) {
        return;
    } 
    lastCheck = millis();

    int voltage = readVoltage() * 100;
    if(voltage < min_voltage || voltage > max_voltage) {
#if DEBUG > 0
        Serial.println("ALARM: Battery voltage dropped");
#endif
        buzzer->alarm();
        return;
    } 
    if(voltage < warn_voltage) {
        if(millis() - lastWarn > 5000) {
#if DEBUG > 0
            Serial.println("WARN: Battery voltage dropped");
#endif
            buzzer->beep(3);
            lastWarn = millis();
        }
    }
}

void BatteryController::updateBatteryBar(float voltage) {
    int used = max_voltage - voltage * 100; // calculate how much voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    float diffPerPixel = voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    float count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine
    
    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

#if DEBUG > 2
    Serial.println("used=" + String(used) + ", value=" + String(value));
    Serial.println("count=" + String(count) + ", diffPerPixel=" + String(diffPerPixel));
    Serial.println("whole=" + String(whole) + ", remainder=" + String(remainder));
#endif

    for(int i=0; i<pixel_count; i++) {
        if(i==whole) {
            int val = calcVal(remainder);
            batPixels.setPixelColor(i, MAX_BRIGHTNESS-val, val, 0);
        }
        if(i>whole) {
            batPixels.setPixelColor(i, 0, 0, 0);
        } 
        if(i<whole) {
            batPixels.setPixelColor(i, 0, MAX_BRIGHTNESS, 0);
        }
        if(value < 0) {
            batPixels.setPixelColor(i, MAX_BRIGHTNESS, 0, 0);
        }
    }
    batPixels.show();
}

int BatteryController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}

int BatteryController::smoothAnalogReading() {
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(BATTERY_PIN);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  return average;
}