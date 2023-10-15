#include <Arduino.h>
#include <Logger.h>

#include "bms_relay.h"
#include "packet.h"
#include "settings.h"
#include "ArduinoJson.h"
#include "CanBus.h"
#include "VescData.h"
#include "VescCanConstants.h"

//https://github.com/lolwheel/Owie/commit/8f10e0897eda7f6e3ca40afeaff909a97b9f7751

// UART RX is connected to the *BMS* White line for OneWheel Pint.
class BMSController {
public:
    BMSController(VescData *vescData);
    VescData *vescData;
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

    boolean isBatteryCellOvercharged(const uint16_t*, int);
    boolean isBatteryCellUndercharged(const uint16_t*, int);
    float batteryCellVariance(const uint16_t*, int);
    boolean isBatteryCellTempMax(const int8_t*, int);
    boolean isBatteryCellTempMin(const int8_t*, int);
    boolean isBMSTempMax(const int8_t);
    boolean isBMSTempMin(const int8_t);

    unsigned long lastBMSData = 0;
    int interval = 5000;

    //quart battery
    float ampHoursSpec=4.25f;
    float ampHoursActual=4.25f;
    float SOC;
    boolean useOverriddenSOC=true;
    float SOH=ampHoursActual/ampHoursSpec*100.0f;
    int cellSeries=15;
    int cellThermistors=4;
    float avgVoltage=cellSeries*3.6;
    float maxVoltage=cellSeries*4.2;
    float wattHours=ampHoursActual*avgVoltage;
    //as far as I can tell these are specs for FM BMS.
    float vCellMin=2.7;
    float vCellMax=4.3;
    float cellTempMax=45;
    //https://www.reddit.com/r/onewheel/comments/knvrh2/battery_temperature_out_of_safety_range/
    float cellTempMin=10;
    float bmsTempMax=60;
    float bmsTempMin=10;
    float cellMaxVarianceSoft=0.00001f;
    float cellMaxVarianceHard=0.00005f;
    boolean chargeOnly=true;
};