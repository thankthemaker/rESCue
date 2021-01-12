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
  // initialize the array for smoothing the analog value
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  batPixels.begin(); // This initializes the NeoPixel library.
}

// Read the voltage from the voltage divider and update the battery bar if connected
float BatteryController::readVoltage() {
    int adc = smoothAnalogReading();  // read the sensor and smooth the value
#ifdef ESP32
    float sensorValue = ( adc * 3.3 ) / (4096);  // calculate the voltage at the ESP32 GPIO
#else
    float sensorValue = ( adc * 3.3 ) / (1024);  // calculate the voltage at the ESP8266 GPIO
#endif
    float voltage = sensorValue *  VOLTAGE_DIVIDER_CONSTANT;  // calculate the battery voltage
#ifdef BATTERY_BAR
    updateBatteryBar(voltage);  // update the WS28xx battery bar
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

// check the voltage against the configured min and max values
void BatteryController::checkVoltage(Buzzer *buzzer) {
    if(millis() - lastCheck < 300) {
        // only check every 300ms
        return;
    } 
    lastCheck = millis();

    int voltage = readVoltage() * 100;
    // check if voltage is below absolute minimum or above absolute maximum (regen)
    if(voltage < min_voltage || voltage > max_voltage) {
#if DEBUG > 0
        Serial.println("ALARM: Battery voltage out of range");
#endif
        buzzer->alarm();  // play an anoying alarm tone
        return;
    } 
    // check if the voltage is close to the minimum
    if(voltage < warn_voltage) {
        if(millis() - lastWarn > 5000) {
#if DEBUG > 0
            Serial.println("WARN: Battery voltage out of range");
#endif
            buzzer->beep(3); // play a warn tonen every 5 seconds
            lastWarn = millis();
        }
    }
}

// updates the battery bar, depending on the LED count
void BatteryController::updateBatteryBar(float voltage) {
    int used = max_voltage - voltage * 100; // calculate how much the voltage has dropped
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

    // update every pixel individually
    for(int i=0; i<pixel_count; i++) {
        if(i==whole) {
            // the last pixel, the battery voltage somewhere in the range of this pixel
            // the lower the remaining value the more the pixel goes from green to red
            int val = calcVal(remainder);
            batPixels.setPixelColor(i, MAX_BRIGHTNESS-val, val, 0);
        }
        if(i>whole) {
            // these pixels must be turned off, we already reached a lower battery voltage 
            batPixels.setPixelColor(i, 0, 0, 0);
        } 
        if(i<whole) {
            // turn on this pixel completely green, the battery voltage is still above this value
            batPixels.setPixelColor(i, 0, MAX_BRIGHTNESS, 0);
        }
        if(value < 0) {
            // ohhh, we already hit the absolute minimum, set all pixel to full red.
            batPixels.setPixelColor(i, MAX_BRIGHTNESS, 0, 0);
        }
    }
    batPixels.show();
}

// map the remaining value to a value between 0 and MAX_BRIGHTNESS
int BatteryController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}

// smoothing the values read from the ADC, there sometimes is some noise
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