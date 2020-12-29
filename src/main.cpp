#include <Arduino.h>
#include "config.h"
#include "ledcontroller.h"
#include "batterycontroller.h"
#include "buzzer.h"
#include "btbridge.h"

int old_forward  = LOW;
int old_backward = LOW;
int old_brake    = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;
int currentVoltage = 0;

LedController *ledController = new LedController();
BatteryController * batController = new BatteryController();
Buzzer *buzzer = new Buzzer();
BluetoothBridge *bridge = new BluetoothBridge();

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif

  pinMode(PIN_FORWARD, INPUT);
  pinMode(PIN_BACKWARD, INPUT);
  pinMode(PIN_BRAKE, INPUT);
  
  // initialize the UART bridge from VESC to Bluetooth
  bridge->init();

  delay(100);
  // initialize the LED (either COB or Neopixel)
  ledController->startSequence(0, 0, MAX_BRIGHTNESS, 100);
  ledController->stop();
}

void loop() {
  new_forward  = digitalRead(PIN_FORWARD);
  new_backward = digitalRead(PIN_BACKWARD);
  new_brake    = digitalRead(PIN_BRAKE);

  // is motor brake active?
  if(new_brake == HIGH) {
    // flash backlights
    ledController->flash(new_forward == LOW);
  } 

  // call the led controller loop
  ledController->loop(&new_forward, &old_forward, &new_backward,&old_backward);       

  // measure and check voltage
  currentVoltage = batController->getVoltage();
  batController->checkVoltage(currentVoltage, buzzer);

  // call the VESC UART-to-Bluetooth bridge
  bridge->loop();

  delay(20);
}