#include <Arduino.h>
#include <Logger.h>
#include "config.h"
#include "BatteryMonitor.h"
#include "Buzzer.h"
#include "SoundController.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#include "BleServer.h"
#include "CanBus.h"

int old_forward  = LOW;
int old_backward = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;

HardwareSerial vesc(2);

Buzzer *buzzer = new Buzzer();
ILedController *ledController = LedControllerFactory::getInstance()->createLedController();

#if defined(CANBUS_ENABLED)
 CanBus * canbus = new CanBus();
 BatteryMonitor *batMonitor = new BatteryMonitor(&canbus->vescData);
#else
 BatteryMonitor *batMonitor = new BatteryMonitor();
#endif //CANBUS_ENABLED

BleServer *bleServer = new BleServer();

// Declare the local logger function before it is called.
void localLogger(Logger::Level level, const char* module, const char* message);

void setup() {
  Logger::setOutputFunction(localLogger);
  Logger::setLogLevel(Logger::NOTICE);
  if(Logger::getLogLevel() != Logger::SILENT) {
    Serial.begin(VESC_BAUD_RATE);
  }

  pinMode(PIN_FORWARD, INPUT);
  pinMode(PIN_BACKWARD, INPUT);
  pinMode(PIN_BRAKE, INPUT);

  vesc.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);      
  delay(50);
#ifdef CANBUS_ENABLED
  // initializes the CANBUS
  canbus->init();
#endif //CANBUS_ENABLED

  // initializes the battery monitor
  batMonitor->init();
  // initialize the UART bridge from VESC to BLE and the BLE support for Blynk (https://blynk.io)
  bleServer->init(&vesc);
  // initialize the LED (either COB or Neopixel)
  ledController->init();

  delay(50);
  ledController->startSequence();
  ledController->stop();
  buzzer->startSequence();
}

void loop() {
#ifdef CANBUS_ENABLED
  new_forward  = canbus->vescData.erpm >= 0 ? 1 : 0;
  new_backward = canbus->vescData.erpm < 0 ? 1 : 0;
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
  batMonitor->checkValues(buzzer);

  // call the VESC UART-to-Bluetooth bridge
#ifdef CANBUS_ENABLED
  canbus->vescData.inputVoltage = random(40, 50);
  bleServer->loop(&canbus->vescData);
#else 
  bleServer->loop();
#endif
}

void localLogger(Logger::Level level, const char* module, const char* message) {
  Serial.print(F("FWC: ["));
  Serial.print(Logger::asString(level));
  Serial.print(F("] "));
  if (strlen(module) > 0) {
      Serial.print(F(": "));
      Serial.print(module);
      Serial.print(" ");
  }
  Serial.println(message);
}