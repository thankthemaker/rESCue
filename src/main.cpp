#include <Arduino.h>
#include "config.h"
#include "BatteryController.h"
#include "Buzzer.h"
#include "BleBridge.h"
#include "ILedController.h"
#include "Ws28xxController.h"

int old_forward  = LOW;
int old_backward = LOW;
int old_brake    = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;
int currentVoltage = 0;
int lastVescValues = 0;

HardwareSerial vesc(2);

ILedController *ledController = LedControllerFactory::getInstance()->createLedController();
BatteryController *batController = new BatteryController();
Buzzer *buzzer = new Buzzer();
BleBridge *bridge = new BleBridge();

void setup() {
#if DEBUG > 0
  Serial.begin(115200);
#endif

  pinMode(PIN_FORWARD, INPUT);
  pinMode(PIN_BACKWARD, INPUT);
  pinMode(PIN_BRAKE, INPUT);

  vesc.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);         
  delay(50);

  // initializes the battery monitor
  batController->init();
  // initialize the UART bridge from VESC to Bluetooth
  bridge->init(&vesc);
  // initialize the LED (either COB or Neopixel)
  ledController->init();

  delay(50);
  ledController->startSequence(0, 0, MAX_BRIGHTNESS, 100);
  ledController->stop();
  buzzer->startSequence();
}

void loop() {
/*
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
  batController->checkVoltage(buzzer);

  // call the VESC UART-to-Bluetooth bridge
  bridge->loop();
*/

  for(float val=50.4; val>40; val=val-0.1) {
    batController->updateBatteryBar(val);
    delay(500);
  }
  delay(5000);
}
