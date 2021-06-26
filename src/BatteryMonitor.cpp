#include "BatteryMonitor.h"

#define LOG_TAG_BATMON "BatteryMonitor"

const int numBatReadings = 15;
const int numCurReadings = 50;
const int min_voltage   = MIN_BATTARY_VOLTAGE;
const int max_voltage   = MAX_BATTARY_VOLTAGE;
const int warn_voltage  = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
const int voltage_range = MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE;
const double max_current= MAX_AVG_CURRENT;
int lastCheck           = 0;
int lastBatWarn         = 0;
int lastCurWarn         = 0;
int readIndex           = 0; // the index of the current reading
int total               = 0; // the running total
int average             = 0; // the average
int batteryReadings[numBatReadings];   // the batteryReadings from the analog input
double currentReadings[numCurReadings];


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

#ifdef LIGHT_BAR_ENABLED
    LightBarController::getInstance()->updateLightBar(
            voltage,mapSwitchState(vescData->switchState, vescData->adc1 > vescData->adc2),
            vescData->erpm);  // update the WS28xx battery bar
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
void BatteryMonitor::checkValues() {
    if(millis() - lastCheck < 300) {
        // only check every 300ms
        return;
    } 
    lastCheck = millis();

    int voltage = readValues() * 100;
    // check if voltage is below absolute minimum or above absolute maximum (regen)
    if(voltage < min_voltage || voltage > max_voltage) {
      Logger::warning(LOG_TAG_BATMON, "ALARM: Battery voltage out of range");
      Buzzer::getInstance()->alarm();  // play an anoying alarm tone
      return;
    } 

    // check if the voltage is close to the minimum
    if(voltage < warn_voltage) {
        if(millis() - lastBatWarn > 5000) {
          Logger::warning(LOG_TAG_BATMON, "WARN: Battery voltage out of range");
          Buzzer::getInstance()->warning(); // play a warn tonen every 5 seconds
          lastBatWarn = millis();
        }
    }

    // check if the average current is higher max
    if(getAverageCurrent() > max_current) {
        if(millis() - lastCurWarn > 5000) {
          Logger::warning(LOG_TAG_BATMON, "WARN: Average current too high");
          Buzzer::getInstance()->warning(); // play a warn tonen every 5 seconds
          lastCurWarn = millis();
        }
    }
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

AdcState BatteryMonitor::mapSwitchState(uint16_t intState, boolean isAdc1Enabled) {
    Serial.printf("Map Switchstate: %d %d\n", intState, isAdc1Enabled);
    switch (intState) {
        case 0:
            return AdcState::ADC_NONE;
        case 1:
            return isAdc1Enabled ? AdcState::ADC_HALF_ADC1 : AdcState::ADC_HALF_ADC2;
        case 2:
            return AdcState::ADC_FULL;
        default:
            Logger::error(LOG_TAG_BATMON, "Unknown switch state");
    }
    return AdcState::ADC_NONE;
}