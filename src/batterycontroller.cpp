#include "BatteryController.h"

int min_voltage = MIN_BATTARY_VOLTAGE;
int max_voltage = MAX_BATTARY_VOLTAGE;
int warn_voltage = MIN_BATTARY_VOLTAGE + (MAX_BATTARY_VOLTAGE - MIN_BATTARY_VOLTAGE) / 10;
int lastWarn = 0;

BatteryController::BatteryController() {}

int BatteryController::readVoltage() {
    int sensorValue = analogRead(BATTERY_PIN);
    float voltage = sensorValue *  VOLTAGE_DIVIDER_CONSTANT;
#if DEBUG > 0
    // print out the value you read:
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
#endif
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