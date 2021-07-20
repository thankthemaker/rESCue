#include <Arduino.h>
#include <Logger.h>
#include "config.h"
#include "BatteryMonitor.h"
#include "Buzzer.h"
////#include "SoundController.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#include "BleServer.h"
#include "CanBus.h"
#include "AppConfiguration.h"
#include "OTA.h"

int new_forward  = LOW;
int new_backward = LOW;
int new_brake    = LOW;
int idle         = LOW;
double idle_erpm = 10.0;

int lastFake = 4000;

HardwareSerial vesc(2);

ILedController *ledController;
OTAUpdater *updater = new OTAUpdater();

#if defined(CANBUS_ENABLED)
 CanBus * canbus = new CanBus();
 BatteryMonitor *batMonitor = new BatteryMonitor(&canbus->vescData);
#else
 BatteryMonitor *batMonitor = new BatteryMonitor();
#endif //CANBUS_ENABLED

BleServer *bleServer = new BleServer();

// Declare the local logger function before it is called.
void localLogger(Logger::Level level, const char* module, const char* message);

void fakeCanbusValues() {
    if(millis() - lastFake > 3000) {
        canbus->vescData.inputVoltage = random(43, 50);
        canbus->vescData.dutyCycle = random(0, 100);
        canbus->vescData.erpm = random(-100, 200);
        canbus->vescData.current = random(0, 10);
        canbus->vescData.ampHours = random(0, 100);
        canbus->vescData.mosfetTemp = random(20, 60);
        canbus->vescData.motorTemp = random(20, 40);
        canbus->vescData.adc1 = 0.5;
        canbus->vescData.adc2 = 0.5;
        canbus->vescData.switchState = 0;
        lastFake = millis();
    }
}

void setup() {
  Logger::setOutputFunction(localLogger);
  Logger::setLogLevel(Logger::WARNING);
  if(Logger::getLogLevel() != Logger::SILENT) {
    Serial.begin(VESC_BAUD_RATE);
  }

  AppConfiguration::getInstance()->readPreferences();

  if(AppConfiguration::getInstance()->config.otaUpdateActive) {
     updater->setup();
     return;
  }

  ledController = LedControllerFactory::getInstance()->createLedController();


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
#ifdef CANBUS_ONLY
  bleServer->init(canbus->stream, canbus);
#else
  bleServer->init(&vesc, canbus);
#endif
  // initialize the LED (either COB or Neopixel)
  ledController->init();

  Buzzer::getInstance()->startSequence();
  ledController->startSequence();

  char buf[128];
  snprintf(buf, 128, " sw-version %d.%d.%d is happily running on hw-version %d.%d", 
    SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH, 
    HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR);
  Logger::notice("rESCue", buf);
}

void loop() {
  if(AppConfiguration::getInstance()->config.otaUpdateActive) {
    return;
  }
#ifdef CANBUS_ENABLED
  #ifdef FAKE_VESC_ENABLED
    fakeCanbusValues();
  #endif

  new_forward  = canbus->vescData.erpm > idle_erpm ? HIGH : LOW;
  new_backward = canbus->vescData.erpm < -idle_erpm ? HIGH : LOW;
  idle         = (abs(canbus->vescData.erpm) < idle_erpm && canbus->vescData.switchState == 0) ? HIGH : LOW;
  new_brake    = (abs(canbus->vescData.erpm) > idle_erpm && canbus->vescData.current < -4.0) ? HIGH : LOW;
#else
  new_forward  = digitalRead(PIN_FORWARD);
  new_backward = digitalRead(PIN_BACKWARD);
  new_brake    = digitalRead(PIN_BRAKE);
  idle         = *(new_forward) == LOW && *(new_backward) == LOW;
#endif

#ifdef CANBUS_ENABLED
  canbus->loop();
#endif

  // is motor brake active?
  if(new_brake == HIGH) {
    // flash backlights
    ledController->changePattern(Pattern::RESCUE_FLASH_LIGHT, new_forward == HIGH, false);
  } 

  // call the led controller loop
  ledController->loop(&new_forward, &new_backward, &idle);    

  // measure and check voltage
  batMonitor->checkValues();

  // call the VESC UART-to-Bluetooth bridge
#ifdef CANBUS_ENABLED
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