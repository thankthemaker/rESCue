#include <Arduino.h>
#include "config.h"
#include "BatteryController.h"
#include "Buzzer.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#ifdef ESP32
 #include "BleBridge.h"
#endif

int old_forward  = LOW;
int old_backward = LOW;
int old_brake    = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;
int currentVoltage = 0;
int lastVescValues = 0;

#ifdef ESP32
HardwareSerial vesc(2);
#endif

ILedController *ledController = LedControllerFactory::getInstance()->createLedController();
BatteryController *batController = new BatteryController();
Buzzer *buzzer = new Buzzer();
#ifdef ESP32
 BleBridge *bridge = new BleBridge();
#endif

void setup() {
#if DEBUG > 0
  Serial.begin(VESC_BAUD_RATE);
#endif

  pinMode(PIN_FORWARD, INPUT);
  pinMode(PIN_BACKWARD, INPUT);
  pinMode(PIN_BRAKE, INPUT);

#ifdef ESP32
  vesc.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);      
#endif
  delay(50);

  // initializes the battery monitor
  batController->init();
#ifdef BLE_ENABLED
  // initialize the UART bridge from VESC to Bluetooth
  bridge->init(&vesc);
#endif
  // initialize the LED (either COB or Neopixel)
  ledController->init();

  delay(50);
  ledController->startSequence(0, 0, MAX_BRIGHTNESS, 100);
  ledController->stop();
  buzzer->startSequence();
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
  batController->checkVoltage(buzzer);

#ifdef ESP32
  // call the VESC UART-to-Bluetooth bridge
  bridge->loop();
#endif
}
