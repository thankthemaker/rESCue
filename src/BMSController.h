#include <Arduino.h>
#include <Logger.h>

#include "bms_relay.h"
#include "packet.h"
#include "settings.h"
#include "ArduinoJson.h"
#include "CanBus.h"

//https://github.com/lolwheel/Owie/commit/8f10e0897eda7f6e3ca40afeaff909a97b9f7751

// UART RX is connected to the *BMS* White line for OneWheel Pint.
class BMSController {
public:
    BMSController();
    void init(CanBus*);
    void loop();
    BmsRelay *relay;
    HardwareSerial bms = HardwareSerial(1);  // Using UART1
    String uptimeString();
    String getTempString();
    String generateOwieStatusJson();
    String getVCellString();
    void broadcastVESCBMS();
private:
    CanBus* canbus_;
    //Map GPIO to trigger IRF540N 100v N-channel mosfet which emulates momentary button for OWIE BMS on (battery voltage on blue wire) to ground. Connect the Source pin of the MOSFET to ground. Connect the Drain pin to the BMS's wake-up wire. Connect the Gate pin to the microcontroller's TX pin.
    void bmsON();
    unsigned long lastBMSData = 0;
    int interval = 5000;

    //quart battery
    float ampHoursSpec=4.25f;
    float ampHoursActual=4.25f;
    float SOC;
    boolean useOverriddenSOC=true;
    float SOH=ampHoursActual/ampHoursSpec*100.0f;
    float cellsSeries=15;
    float avgVoltage=cellsSeries*3.6;
    float maxVoltage=cellsSeries*4.2;
    float wattHours=ampHoursActual*avgVoltage;
    //as far as I can tell these are specs for FM BMS.
    float vCellMin=2.6;
    float vCellMax=4.2;
    float cellMaxTemp=50;
};