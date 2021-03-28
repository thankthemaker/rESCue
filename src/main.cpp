#include <Arduino.h>
#include <Logger.h>
#include "config.h"
#include "BatteryMonitor.h"
#include "Buzzer.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#ifdef ESP32
 #include "BleServer.h"
 #include "CanBus.h"
#endif //ESP32

int old_forward  = LOW;
int old_backward = LOW;
int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;

#ifdef ESP32
 HardwareSerial vesc(2);
#endif //ESP32

Buzzer *buzzer = new Buzzer();
ILedController *ledController = LedControllerFactory::getInstance()->createLedController();

#if defined(CANBUS_ENABLED) && defined(ESP32)
 CanBus * canbus = new CanBus();
 BatteryMonitor *batMonitor = new BatteryMonitor(&canbus->vescData);
#else
 BatteryMonitor *batMonitor = new BatteryMonitor();
#endif //CANBUS_ENABLED && ESP32

#ifdef ESP32
 BleServer *bleServer = new BleServer();
#endif //ESP32

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
#endif //ESP32

  // initializes the battery monitor
  batMonitor->init();
#ifdef ESP32
  // initialize the UART bridge from VESC to Bluetooth
  bleServer->init(&vesc);
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

  // measure and check values (voltage, current)
  batMonitor->checkValues(buzzer);

#ifdef ESP32
  // call the VESC UART-to-Bluetooth bridge
  canbus->vescData.inputVoltage = random(400, 504)/10.0;
  canbus->vescData.erpm = random(5000, 8000);
  canbus->vescData.dutyCycle = random(-100, 100);
  bleServer->loop(&canbus->vescData);
#endif //ESP32
}

void localLogger(Logger::Level level, const char* module, const char* message) {
  Serial.print(F("FWC: ["));

  Serial.print(Logger::asString(level));

  Serial.print(F("] "));

  if (strlen(module) > 0)
  {
      Serial.print(F(": "));
      Serial.print(module);
      Serial.print(" ");
  }

  Serial.println(message);
}