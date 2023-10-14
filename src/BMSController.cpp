#include "BMSController.h"

#ifndef BMS_TX_PIN
#define BMS_TX_PIN 3 
#endif

#ifndef BMS_ON_PIN
#define BMS_ON_PIN 16 
#endif

BMSController::BMSController() {}

void BMSController::init(CanBus* canbus) {
  canbus_=canbus;
  relay = new BmsRelay([this]() { return bms.read(); },
                       [](uint8_t b) {
                       },
                       millis);
bms.begin(115200, SERIAL_8N1, BMS_TX_PIN, -1);

  pinMode(BMS_ON_PIN, OUTPUT);

  relay->addReceivedPacketCallback([this](BmsRelay *, Packet *packet) {
    //do something with data. this is where we can construct a can packet for VESC BMS
    unsigned long now = millis();
    if(canbus_->isInitialized() && (lastBMSData <= now && now - lastBMSData > canbus_->getInterval()) && !canbus_->proxy->processing)
    {
    broadcastVESCBMS();
    lastBMSData = (millis() + 500);
    }
  });
  relay->setUnknownDataCallback([this](uint8_t b) {
    static std::vector<uint8_t> unknownData = {0};
    if (unknownData.size() > 128) {
      return;
    }
    //unknownData.push_back(b);
    //do something with unknown data
    //streamBMSPacket(&unknownData[0], unknownData.size());
  });
  //relay->setBMSSerialOverride(0xFFABCDEF);
}

void BMSController::loop()
{
  //keep track of time and if no packet has come through in awhile try toggling bmsON();
  unsigned long now = millis();
  if(lastBMSData <= now && now - lastBMSData > interval)
  {
    bmsON();
    lastBMSData = now;
  }
  relay->loop();
}

void BMSController::broadcastVESCBMS()
{
  SOC = (useOverriddenSOC) ? relay->getOverriddenSOC() : relay->getBmsReportedSOC();

  //String json=generateOwieStatusJson(); //one step at a time but we can start by grabbing json blob of OWIE status. Can print it out.
  //if(json &&json.length()!=0){
  //  DynamicJsonDocument doc(1024);
  //  deserializeJson(doc, json);
  //  if (doc["TOTAL_VOLTAGE"]&&strcmp(doc["TOTAL_VOLTAGE"], "0.00v") != 0) {
  //    printf("%s\n",json.c_str());
  //  }
  //}
  canbus_->bmsAHWH(ampHoursActual,ampHoursActual*avgVoltage);
  canbus_->bmsVTOT(relay->getTotalVoltageMillivolts() / 1000.0f,relay->isCharging() ? maxVoltage : 0.0f);

  canbus_->bmsI(relay->getCurrentMilliamps() / 1000.0f > 0.0f ? relay->getCurrentMilliamps() / 1000.0f : 0.0f);


  boolean isBalancing;
  if (relay->isCharging() && relay->getBmsReportedSOC()>=99)
  {
    isBalancing=true;
  }
  else
  {
    isBalancing=false;
  }

  boolean isChargeAllowed;
  if(!relay->isBatteryTempOutOfRange() && !relay->isBatteryOvercharged())
  {
    isChargeAllowed=true;
  }
  else
  {
    isChargeAllowed=false;
  }

  boolean isChargeOk;
  if(relay->isCharging() && !relay->isBatteryTempOutOfRange() && !relay->isBatteryOvercharged())
  {
    isChargeOk=true;
  }
  {
    isChargeOk=false;
  }

  canbus_->bmsSOCSOHTempStat(vCellMin,vCellMax,SOC,SOH,cellMaxTemp,relay->isCharging(),isBalancing,isChargeAllowed,isChargeOk);

  canbus_->bmsAHWHDischargeTotal(relay->getUsedChargeMah() / 1000.0,relay->getUsedChargeMah() / 1000.0*avgVoltage);

  canbus_->bmsAHWHChargeTotal(relay->getRegeneratedChargeMah() / 1000.0,relay->getRegeneratedChargeMah() / 1000.0*avgVoltage);

  const uint16_t *cellMillivolts = relay->getCellMillivolts();
  canbus_->bmsVCell(cellMillivolts);

  const int8_t *thermTemps = relay->getTemperaturesCelsius();
  canbus_->bmsTemps(thermTemps);

  //maybe make this update only when there's an actual change
  canbus_->bmsBal(isBalancing);

  bms_op_state op_state=BMS_OP_STATE_INIT;
  bms_fault_state fault_state=BMS_FAULT_CODE_NONE;
  if(relay->isCharging()) op_state=BMS_OP_STATE_CHARGING;
  if(isBalancing) op_state=BMS_OP_STATE_BALANCING;
  //if(relay->isBatteryEmpty()) op_state=BMS_OP_STATE_BATTERY_DEAD; //not sure if we want this if it's using the default SoC.
  if(relay->isBatteryOvercharged()) fault_state=BMS_FAULT_CODE_PACK_OVER_VOLTAGE;
  if(relay->isBatteryTempOutOfRange()) fault_state=BMS_FAULT_CODE_CHARGE_OVER_TEMP_CELLS;

  //well that's all that can be done from the state flags sent from the BMS. For other BMS FAULT codes we'll have to calculate them manually I think.
  canbus_->bmsState(op_state, fault_state);
}

String BMSController::uptimeString() {
  const unsigned long nowSecs = millis() / 1000;
  const int hrs = nowSecs / 3600;
  const int mins = (nowSecs % 3600) / 60;
  const int secs = nowSecs % 60;
  String ret;
  if (hrs) {
    ret.concat(hrs);
    ret.concat('h');
  }
  ret.concat(mins);
  ret.concat('m');
  ret.concat(secs);
  ret.concat('s');
  return ret;
}

String BMSController::getTempString() {
  const int8_t *thermTemps = relay->getTemperaturesCelsius();
  String temps;
  temps.reserve(256);
  temps.concat("<tr>");
  for (int i = 0; i < 5; i++) {
    temps.concat("<td>");
    temps.concat(thermTemps[i]);
    temps.concat("</td>");
  }
  temps.concat("<tr>");
  return temps;
}

String BMSController::getVCellString()
{
  const uint16_t *cellMillivolts = relay->getCellMillivolts();
  String vcells;
  vcells.reserve(256);
  for (int i = 0; i < 3; i++) {
    vcells.concat("<tr>");
    for (int j = 0; j < 5; j++) {
      vcells.concat("<td>");
      vcells.concat(cellMillivolts[i * 5 + j] / 1000.0);
      vcells.concat("</td>");
    }
    vcells.concat("<tr>");
  }
  return vcells;
}


String BMSController::generateOwieStatusJson() {
  DynamicJsonDocument status(1024);
  String jsonOutput;

  status["TOTAL_VOLTAGE"] =
      String(relay->getTotalVoltageMillivolts() / 1000.0, 2) + "v";
  status["CURRENT_AMPS"] =
      String(relay->getCurrentMilliamps() / 1000.0, 1) + " Amps";
  status["BMS_SOC"] = String(relay->getBmsReportedSOC()) + "%";
  status["OVERRIDDEN_SOC"] = String(relay->getOverriddenSOC()) + "%";
  status["USED_CHARGE_MAH"] = String(relay->getUsedChargeMah()) + " mAh";
  status["REGENERATED_CHARGE_MAH"] =
      String(relay->getRegeneratedChargeMah()) + " mAh";
  status["UPTIME"] = uptimeString();
  status["CELL_VOLTAGE_TABLE"] = getVCellString();
  status["TEMPERATURE_TABLE"] = getTempString();

  serializeJson(status, jsonOutput);
  return jsonOutput;
}

//Map GPIO TX to trigger IRF540N 100v N-channel mosfet which emulates momentary button for OWIE BMS on (battery voltage on blue wire) to ground. Connect the Source pin of the MOSFET to ground. Connect the Drain pin to the BMS's wake-up wire. Connect the Gate pin to the microcontroller's TX pin.
void BMSController::bmsON() {
  //turn on BMS
  digitalWrite(BMS_ON_PIN, HIGH);  // Turn MOSFET ON
  delay(10);                      
  digitalWrite(BMS_ON_PIN, LOW);   // Turn MOSFET OFF
}