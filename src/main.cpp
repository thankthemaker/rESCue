#include <Arduino.h>
#include "config.h"
#include "BatteryController.h"
#include "Buzzer.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#ifdef ESP32
 #include "BleBridge.h"
 #include "CanBus.h"
#endif //ESP32

int old_forward  = LOW;
int old_backward = LOW;
int old_brake    = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;
int currentVoltage = 0;
int lastVescValues = 0;

int lastFade = 0;
int count = 0;

#ifdef ESP32
 HardwareSerial vesc(2);
#endif //ESP32

Buzzer *buzzer = new Buzzer();
ILedController *ledController = LedControllerFactory::getInstance()->createLedController();

#if defined(CANBUS_ENABLED) && defined(ESP32)
 CanBus * canbus = new CanBus();
 BatteryController *batController = new BatteryController(&canbus->vescData);
#else
 BatteryController *batController = new BatteryController();
#endif //CANBUS_ENABLED && ESP32

#ifdef ESP32
 BleBridge *bridge = new BleBridge();
#endif //ESP32

void setup() {
#if DEBUG > 0
  Serial.begin(VESC_BAUD_RATE);
#endif

  pinMode(PIN_FORWARD, INPUT);
  pinMode(PIN_BACKWARD, INPUT);
  pinMode(PIN_BRAKE, INPUT);

#ifdef CANBUS_ENABLED
  vesc.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);      
  delay(50);
  // initializes the CANBUS
  canbus->init();
#endif //ESP32

  // initializes the battery monitor
  batController->init();
#ifdef ESP32
  // initialize the UART bridge from VESC to Bluetooth
  bridge->init(&vesc);
#endif //ESP32
  // initialize the LED (either COB or Neopixel)
  ledController->init();

  delay(50);
  ledController->startSequence();
  ledController->stop();
  buzzer->startSequence();
}

void loop() {
#ifdef CANBUS_ENABLED
  new_forward  = canbus->vescData.dutyCycle >= 0 ? 1 : 0;
  new_backward = canbus->vescData.dutyCycle < 0 ? 1 : 0;
  new_brake    = 0;
#else
  new_forward  = digitalRead(PIN_FORWARD);
  new_backward = digitalRead(PIN_BACKWARD);
  new_brake    = digitalRead(PIN_BRAKE);
#endif

#ifdef CANBUS_ENABLED
  canbus->loop();
#endif

  // is motor brake active?
  if(new_brake == HIGH) {
    // flash backlights
    ledController->flash(&new_forward);
  } 

  // call the led controller loop
  ledController->loop(&new_forward, &old_forward, &new_backward,&old_backward);    

  // measure and check voltage
  batController->checkVoltage(buzzer);

#ifdef ESP32
  // call the VESC UART-to-Bluetooth bridge
  bridge->loop();
#endif //ESP32
}
