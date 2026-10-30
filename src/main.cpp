#include <Arduino.h>
#include "esp_log.h"
#include "config.h"
#include "IVescCommunication.h"
#include "BatteryMonitor.h"
#include "Buzzer.h"
#include "ILedController.h"
#include "Ws28xxController.h"
#include "BleServer.h"
#include "AppConfiguration.h"
#include "LightBarController.h"
#include <ble_ota_dfu.hpp>

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
  #include "BMSController.h"
  #include "CanBus.h"
#endif

const int mainBufSize = 128;
char mainBuf[mainBufSize];

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

ILedController *ledController;
IVescCommunication *vescCommunication;

BatteryMonitor *batMonitor = new BatteryMonitor(&vescData);
BleServer *bleServer = new BleServer();
LightBarController *lightbar = new LightBarController();

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
  BMSController *bmsController = new BMSController(&vescData);
#endif

void initializePinDefinitions() {
    // Debug LED on board
#ifdef PIN_BOARD_LED
    pinMode(PIN_BOARD_LED, OUTPUT);
    digitalWrite(PIN_BOARD_LED, LOW);
#endif
#if defined(PIN_FORWARD) && defined(PIN_BACKWARD) && defined(PIN_BRAKE)
    pinMode(PIN_FORWARD, INPUT);
    pinMode(PIN_BACKWARD, INPUT);
    pinMode(PIN_BRAKE, INPUT);
#endif
}

/**
 * Initializes the VESC communication and all sub components.
 * The concrete transport (CANBUS or UART) is chosen by the factory.
 */
void initializeComponents() {
    delay(50);
    vescCommunication->init();
    delay(50);

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
    bmsController->init(static_cast<CanBus *>(vescCommunication));
    delay(50);
#endif

    // initializes the battery monitor
    batMonitor->init();
    // initialize the BLE support for the rESCue-App and the transparent VESC bridge
    delay(50);
    bleServer->init(vescCommunication);
    // initialize the LED (either COB or Neopixel)
    ledController->init();
    delay(50);
    lightbar->init();
}

/**
 * Calculates the current loop time and the maximum loop time.
 */
void calculateLoopTimes() {
    loopTime = millis() - mainLoop;
    mainLoop = millis();
    if (loopTime > maxLoopTime) {
        maxLoopTime = loopTime;
    }
}

/**
 * Checks if an OTA firmware update should be started.
 * If not already started, a short melody indicates OTA mode, the BLE stack is shut down
 * and the update procedure (implemented in the "ble_ota_dfu" library) is started.
 * @return true while an OTA update is active, so the caller must not run the normal loop.
 */
boolean handleFirmwareUpdate() {
    if (!AppConfiguration::getInstance()->config.otaUpdateActive) {
        return false;
    }
    if (!updateInProgress) {
        Buzzer::startUpdateSequence();
        bleServer->stop();
        ota_dfu_ble.begin(AppConfiguration::getInstance()->config.deviceName.c_str());
        updateInProgress = true;
    }
    return true;
}

/**
 * Handles a request for saving and sending the configuration.
 * Called from the main loop because BLE callbacks must return fast.
 */
void handleConfigurationRequests() {
    if (AppConfiguration::getInstance()->config.sendConfig) {
        BleServer::sendConfig();
        AppConfiguration::getInstance()->config.sendConfig = false;
    }
    if (AppConfiguration::getInstance()->config.saveConfig) {
        AppConfiguration::getInstance()->savePreferences();
        AppConfiguration::getInstance()->config.saveConfig = false;
    }
}

/**
 * Derives the direction/brake/idle signals, either from the values the VESC sent
 * (CANBUS) or from the dedicated Cheap FOCer pins.
 */
void readVescValues() {
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
}

/**
 * Lets every sub component do its work within a single main loop iteration.
 * Don't block or use delay within the sub components - it would hurt performance and stability.
 */
void handleComponentLoops() {
    readVescValues();

    vescCommunication->loop();

#if defined(CANBUS_ENABLED) && defined(BMS_TX_PIN) && defined(BMS_ON_PIN)
    bmsController->loop();
#endif

    // call the led controller loop
    ledController->loop(&new_forward, &new_backward, &idle, &new_brake, &mall_grab);

    // measure and check voltage
    batMonitor->checkValues();

    // update the WS28xx battery bar
    lightbar->updateLightBar(vescData.inputVoltage, vescData.switchState, vescData.adc1, vescData.adc2, vescData.erpm);

    // call the VESC-to-Bluetooth bridge
    bleServer->loop(&vescData, loopTime, maxLoopTime);
}

void setup() {
    delay(500);

    initializePinDefinitions();

    AppConfiguration::getInstance()->readPreferences();
    delay(10);
    AppConfiguration::getInstance()->config.sendConfig = false;

    if (esp_log_level_get("*") != ESP_LOG_NONE) {
#ifdef ESP32S3
        Serial.setRxBufferSize(2048);
#endif
        Serial.begin(VESC_BAUD_RATE);
    }

    ledController = LedControllerFactory::getInstance()->createLedController(&vescData);
    vescCommunication = VescCommunicationFactory::getInstance()->createVescCommunication(&vescData);

    initializeComponents();

    Buzzer::startSequence();
    ledController->startSequence();

    snprintf(mainBuf, mainBufSize, " sw-version %d.%d.%d is happily running on hw-version %d.%d",
             SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH,
             HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR);
    ESP_LOGI("rESCue", "%s", mainBuf);

#ifdef PIN_BOARD_LED
    digitalWrite(PIN_BOARD_LED, HIGH);
#endif
}

void loop() {
    calculateLoopTimes();

    // during an OTA update nothing else is allowed to run
    if (handleFirmwareUpdate()) {
        return;
    }

    handleConfigurationRequests();
    handleComponentLoops();
}
