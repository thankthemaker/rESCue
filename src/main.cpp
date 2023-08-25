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

unsigned long mainLoop = 0;
unsigned long loopTime = 0;
unsigned long maxLoopTime = 0;
int new_forward = LOW;
int new_backward = LOW;
int new_brake = LOW;
int idle = LOW;
double idle_erpm = 10.0;

int lastFake = 4000;
int lastFakeCount = 0;
VescData vescData;

HardwareSerial vesc(2);

ILedController *ledController;

#if defined(CANBUS_ENABLED)
CanBus *canbus = new CanBus(&vescData);
#endif //CANBUS_ENABLED
BatteryMonitor *batMonitor = new BatteryMonitor(&vescData);

BleServer *bleServer = new BleServer();

// Declare the local logger function before it is called.
void localLogger(Logger::Level level, const char *module, const char *message);

#if defined(CANBUS_ENABLED)

void fakeCanbusValues() {
    if (millis() - lastFake > 3000) {
        vescData.tachometer = random(0, 30);
        vescData.inputVoltage = random(43, 50);
        vescData.dutyCycle = random(0, 100);
        if (lastFakeCount > 10) {
            vescData.erpm = random(-100, 200);
        } else {
            vescData.erpm = 0;//random(-100, 200);
        }
        vescData.current = random(0, 10);
        vescData.ampHours = random(0, 100);
        vescData.mosfetTemp = random(20, 60);
        vescData.motorTemp = random(20, 40);
        vescData.adc1 = 0.5;
        vescData.adc2 = 0.5;
        vescData.switchState = 0;
        lastFake = millis();
        lastFakeCount++;
    }
}

#endif

void setup() {

//Debug LED on board
#ifdef PIN_BOARD_LED
    pinMode(PIN_BOARD_LED,OUTPUT);
    digitalWrite(PIN_BOARD_LED,LOW);
#endif

    Logger::setOutputFunction(localLogger);

    AppConfiguration::getInstance()->readPreferences();
    delay(3);
    AppConfiguration::getInstance()->config.sendConfig = false;
    Logger::setLogLevel(AppConfiguration::getInstance()->config.logLevel);
    if (Logger::getLogLevel() != Logger::SILENT) {
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
    bleServer->init(&vesc);
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
        return;
    }
    if (AppConfiguration::getInstance()->config.sendConfig) {
        bleServer->sendConfig();
        AppConfiguration::getInstance()->config.sendConfig = false;
    }
    if (AppConfiguration::getInstance()->config.saveConfig) {
        AppConfiguration::getInstance()->savePreferences();
        AppConfiguration::getInstance()->config.saveConfig = false;
    }

#ifdef CANBUS_ENABLED
#ifdef FAKE_VESC_ENABLED
    fakeCanbusValues();
#endif

    new_forward = vescData.erpm > idle_erpm ? HIGH : LOW;
    new_backward = vescData.erpm < -idle_erpm ? HIGH : LOW;
    idle = (abs(vescData.erpm) < idle_erpm && vescData.switchState == 0) ? HIGH : LOW;
    new_brake = (abs(vescData.erpm) > idle_erpm && vescData.current < -4.0) ? HIGH : LOW;
#else
    new_forward  = digitalRead(PIN_FORWARD);
    new_backward = digitalRead(PIN_BACKWARD);
    new_brake    = digitalRead(PIN_BRAKE);
    idle         = new_forward == LOW && new_backward == LOW;
#endif

#ifdef CANBUS_ENABLED
#ifndef FAKE_VESC_ENABLED
    canbus->loop();
#endif
#endif

    // call the led controller loop
    ledController->loop(&new_forward, &new_backward, &idle, &new_brake);

    // measure and check voltage
    batMonitor->checkValues();

    // call the VESC UART-to-Bluetooth bridge
#ifdef CANBUS_ENABLED
    bleServer->loop(&vescData, loopTime, maxLoopTime);
#else
    bleServer->loop();
#endif
}

void localLogger(Logger::Level level, const char *module, const char *message) {
    Serial.print(F("["));
    Serial.print(Logger::asString(level));
    Serial.print(F("] "));
    if (strlen(module) > 0) {
        Serial.print(F(": "));
        Serial.print(module);
        Serial.print(" ");
    }
    Serial.println(message);
}