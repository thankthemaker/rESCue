#include "BatteryController.h"

int min_voltage = MIN_BATTARY_VOLTAGE;
int max_voltage = MAX_BATTARY_VOLTAGE;
int warn_voltage = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
int lastWarn = 0;

BatteryController::BatteryController() {}

int BatteryController::getVoltage() {
  // TODO
  return 4200;  
}

void BatteryController::checkVoltage(int voltage, Buzzer *buzzer) {
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