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
#include "LightBarController.h"
#include <ble_ota_dfu.hpp>

const int mainBufSize = 128;
char mainBuf[mainBufSize];

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
  #include "BMSController.h"
#endif

unsigned long mainLoop = 0;
unsigned long loopTime = 0;
unsigned long maxLoopTime = 0;
int new_forward = LOW;
int new_backward = LOW;
int new_brake = LOW;
int idle = LOW;
int mall_grab = LOW;
double idle_erpm = 10.0;
boolean updateInProgress = false;

BLE_OTA_DFU ota_dfu_ble;

VescData vescData;

#ifndef CANBUS_ENABLED
  HardwareSerial vesc(2);
#endif

ILedController *ledController;

#if defined(CANBUS_ENABLED)
CanBus *canbus = new CanBus(&vescData);
#endif //CANBUS_ENABLED
BatteryMonitor *batMonitor = new BatteryMonitor(&vescData);

BleServer *bleServer = new BleServer();
LightBarController *lightbar = new LightBarController();
// Declare the local logger function before it is called.
void localLogger(Logger::Level level, const char *module, const char *message);

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
  BMSController *bmsController = new BMSController(&vescData);
#endif

void setup() {

//Debug LED on board
#ifdef PIN_BOARD_LED
    pinMode(PIN_BOARD_LED,OUTPUT);
    digitalWrite(PIN_BOARD_LED,LOW);
#endif

    Logger::setOutputFunction(localLogger);

    AppConfiguration::getInstance()->readPreferences();
 //   AppConfiguration::getInstance()->readMelodies();
    delay(10);
    AppConfiguration::getInstance()->config.sendConfig = false;
    Logger::setLogLevel(AppConfiguration::getInstance()->config.logLevel);
    if (Logger::getLogLevel() != Logger::SILENT) {
#ifdef ESP32S3
        Serial.setRxBufferSize(2048);
#endif
        Serial.begin(VESC_BAUD_RATE);
    }

    if (AppConfiguration::getInstance()->config.otaUpdateActive) {
        return;
    }

    ledController = LedControllerFactory::getInstance()->createLedController(&vescData);

    #if defined(PIN_FORWARD) && defined(PIN_BACKWARD) && defined(PIN_BRAKE)
    pinMode(PIN_FORWARD, INPUT);
    pinMode(PIN_BACKWARD, INPUT);
    pinMode(PIN_BRAKE, INPUT);
    #endif

//    vesc.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);
    delay(50);
#ifdef CANBUS_ENABLED
    // initializes the CANBUS
    canbus->init();
#endif //CANBUS_ENABLED

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
    bmsController->init(canbus);
#endif

    // initializes the battery monitor
    batMonitor->init();
    // initialize the UART bridge from VESC to BLE and the BLE support for Blynk (https://blynk.io)
#ifdef CANBUS_ONLY
    bleServer->init(canbus->stream, canbus);
#else
    bleServer->init(&vesc);
#endif
    // initialize the LED (either COB or Neopixel)
    ledController->init();

    Buzzer::startSequence();
    ledController->startSequence();

    snprintf(mainBuf, mainBufSize, " sw-version %d.%d.%d is happily running on hw-version %d.%d",
             SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH,
             HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR);
    Logger::notice("rESCue", mainBuf);

#ifdef PIN_BOARD_LED
    digitalWrite(PIN_BOARD_LED,HIGH);
#endif
}

void loop() {
    loopTime = millis() - mainLoop;
    mainLoop = millis();
    if (loopTime > maxLoopTime) {
        maxLoopTime = loopTime;
    }

    if (AppConfiguration::getInstance()->config.otaUpdateActive) {
        if(!updateInProgress) {
            Buzzer::startUpdateSequence();
            bleServer->stop();
            ota_dfu_ble.begin(AppConfiguration::getInstance()->config.deviceName.c_str()); 
            updateInProgress = true;
        }
        return;
    }

    if (AppConfiguration::getInstance()->config.sendConfig) {
        BleServer::sendConfig();
        AppConfiguration::getInstance()->config.sendConfig = false;
    }
    if (AppConfiguration::getInstance()->config.saveConfig) {
        AppConfiguration::getInstance()->savePreferences();
        //AppConfiguration::getInstance()->saveMelodies();
        AppConfiguration::getInstance()->config.saveConfig = false;
    }

#ifdef CANBUS_ENABLED
    new_forward = vescData.erpm > idle_erpm ? HIGH : LOW;
    new_backward = vescData.erpm < -idle_erpm ? HIGH : LOW;
    idle = (abs(vescData.erpm) < idle_erpm && vescData.switchState == 0) ? HIGH : LOW;
    new_brake = (abs(vescData.erpm) > idle_erpm && vescData.current < -4.0) ? HIGH : LOW;
    mall_grab = (vescData.pitch > 70.0) ? HIGH : LOW;
#else
    new_forward  = digitalRead(PIN_FORWARD);
    new_backward = digitalRead(PIN_BACKWARD);
    new_brake    = digitalRead(PIN_BRAKE);
    idle         = new_forward == LOW && new_backward == LOW;
    mall_grab    = LOW;
#endif

#ifdef CANBUS_ENABLED
    canbus->loop();
#endif

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
    bmsController->loop();
#endif

    // call the led controller loop
    ledController->loop(&new_forward, &new_backward, &idle, &new_brake, &mall_grab);

    // measure and check voltage
    batMonitor->checkValues();

    lightbar->updateLightBar(vescData.inputVoltage, vescData.switchState, vescData.adc1, vescData.adc2, vescData.erpm);  // update the WS28xx battery bar

    // call the VESC UART-to-Bluetooth bridge
    bleServer->loop(&vescData, loopTime, maxLoopTime);
}

void localLogger(Logger::Level level, const char *module, const char *message) {
    log_printf(F("["));
    log_printf(Logger::asString(level));
    log_printf(F("] "));
    if (strlen(module) > 0) {
        log_printf(F(": "));
        log_printf(module);
        log_printf(" ");
    }
    log_printf(message);
    log_printf("\n");
}