#include "BatteryMonitor.h"
#include <Logger.h>

#define LOG_TAG_BATMON "BatteryMonitor"

const int numBatReadings = 50;
const int numCurReadings = 50;
const int min_voltage   = MIN_BATTARY_VOLTAGE;
const int max_voltage   = MAX_BATTARY_VOLTAGE;
const int warn_voltage  = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
const int voltage_range = MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE;
const int pixel_count   = BATTERY_BAR_NUMPIXELS;
const double max_current= MAX_AVG_CURRENT;
int lastCheck           = 0;
int lastBatWarn         = 0;
int lastCurWarn         = 0;
int readIndex           = 0; // the index of the current reading
int total               = 0; // the running total
int average             = 0; // the average
int batteryReadings[numBatReadings];   // the batteryReadings from the analog input
double currentReadings[numCurReadings];

#ifdef BATTERY_BAR
 Adafruit_NeoPixel batPixels = Adafruit_NeoPixel(BATTERY_BAR_NUMPIXELS, BATTERY_BAR_PIN, NEO_GRB + NEO_KHZ800);
#endif

BatteryMonitor::BatteryMonitor() {}
#ifdef CANBUS_ENABLED
BatteryMonitor::BatteryMonitor(CanBus::VescData *vescData) {
  this->vescData = vescData;
}
#endif


void BatteryMonitor::init() {
  Logger::notice(LOG_TAG_BATMON, "initializing...");

  // initialize the array for smoothing the analog value
  for (int i = 0; i < numBatReadings; i++) {
    batteryReadings[i] = 0;
  }
  for (int i = 0; i < numCurReadings; i++) {
    currentReadings[i] = 0;
  }
  batPixels.begin(); // This initializes the NeoPixel library.
}

// Read the voltage from the voltage divider and update the battery bar if connected
float BatteryMonitor::readValues() {
#ifndef CANBUS_ENABLED
    int adc = smoothAnalogReading();  // read the sensor and smooth the value
    float sensorValue = ( adc * 3.3 ) / (4096);  // calculate the voltage at the ESP32 GPIO
    float voltage = sensorValue *  VOLTAGE_DIVIDER_CONSTANT;  // calculate the battery voltage
#else
    float voltage = vescData->inputVoltage; 
    float current = abs(vescData->current);
    updateCurrentArray(current);
#endif //CANBUS_ENABLED

#ifdef BATTERY_BAR
    updateBatteryBar(voltage);  // update the WS28xx battery bar
#endif
  if(Logger::getLogLevel() == Logger::VERBOSE) {
#ifndef CANBUS_ENABLED
    Logger::verbose(LOG_TAG_BATMON, String("ADC: " + String(adc)).c_str());
    Logger::verbose(LOG_TAG_BATMON, String("Sensorvalue: " + String(sensorValue) + "V").c_str());
#endif
    Logger::verbose(LOG_TAG_BATMON, String("Voltage: " + String(voltage) + "V").c_str());
  }
  return voltage;
}

// check the voltage against the configured min and max values
void BatteryMonitor::checkValues(Buzzer *buzzer) {
    if(millis() - lastCheck < 300) {
        // only check every 300ms
        return;
    } 
    lastCheck = millis();

    int voltage = readValues() * 100;
    // check if voltage is below absolute minimum or above absolute maximum (regen)
    if(voltage < min_voltage || voltage > max_voltage) {
      Logger::warning(LOG_TAG_BATMON, "ALARM: Battery voltage out of range");
      buzzer->alarm();  // play an anoying alarm tone
      return;
    } 

    // check if the voltage is close to the minimum
    if(voltage < warn_voltage) {
        if(millis() - lastBatWarn > 5000) {
          Logger::warning(LOG_TAG_BATMON, "WARN: Battery voltage out of range");
          buzzer->beep(3); // play a warn tonen every 5 seconds
          lastBatWarn = millis();
        }
    }

    // check if the average current is higher max
    if(getAverageCurrent() > max_current) {
        if(millis() - lastCurWarn > 5000) {
          Logger::warning(LOG_TAG_BATMON, "WARN: Average current too high");
          buzzer->beep(3); // play a warn tonen every 5 seconds
          lastCurWarn = millis();
        }
    }
}

// updates the battery bar, depending on the LED count
void BatteryMonitor::updateBatteryBar(float voltage) {
    int used = max_voltage - voltage * 100; // calculate how much the voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    float diffPerPixel = voltage_range / 100.0 / pixel_count; // calculate how much voltage a single pixel shall represent
    float count = value / 100.0 / diffPerPixel; // calculate how many pixels must shine
    
    int whole = count; // number of "full" green pixels
    int remainder = (count - whole) * 100; // percentage of usage of current pixel

    if(Logger::getLogLevel() == Logger::VERBOSE) {
      Logger::verbose(LOG_TAG_BATMON, String("used=" + String(used) + ", value=" + String(value)).c_str());
      Logger::verbose(LOG_TAG_BATMON, String("count=" + String(count) + ", diffPerPixel=" + String(diffPerPixel)).c_str());
      Logger::verbose(LOG_TAG_BATMON, String("whole=" + String(whole) + ", remainder=" + String(remainder)).c_str());
    }

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
int BatteryMonitor::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}

// smoothing the values read from the ADC, there sometimes is some noise
int BatteryMonitor::smoothAnalogReading() {
  total = total - batteryReadings[readIndex];
  // read from the sensor:
  batteryReadings[readIndex] = analogRead(BATTERY_PIN);
  // add the reading to the total:
  total = total + batteryReadings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numBatReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numBatReadings;
  return average;
}

void BatteryMonitor::updateCurrentArray(double value) {
  for(int i=numCurReadings; i>0; i--) {
      currentReadings[i] = currentReadings[i-1];
  }
  currentReadings[0] = value;
}

double BatteryMonitor::getAverageCurrent() {
    double avg = 0;
    for(int i=0; i<numCurReadings; i++) {
      avg += currentReadings[i];
    }
    return avg/numCurReadings;
}