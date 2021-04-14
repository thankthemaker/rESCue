#include "CanBus.h"
#include <Logger.h>

#ifdef CANBUS_ENABLED

#ifndef VESC_CAN_ID
 #define VESC_CAN_ID 25
#endif
#ifndef ESP_CAN_ID
 #define ESP_CAN_ID 3
#endif

int lastDump = 0;
int lastRealtimeData = 0;
int lastBalanceData = 0;
uint32_t STATUS_1 = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS) << 8) + VESC_CAN_ID;
uint32_t STATUS_2 = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_2) << 8) + VESC_CAN_ID;
uint32_t STATUS_3 = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_3) << 8) + VESC_CAN_ID;
uint32_t STATUS_4 = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_4) << 8) + VESC_CAN_ID;
uint32_t STATUS_5 = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_5) << 8) + VESC_CAN_ID;
uint32_t FILL_RX_BUFFER = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_FILL_RX_BUFFER) << 8) + VESC_CAN_ID;
uint32_t PROCESS_RX_BUFFER = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_PROCESS_RX_BUFFER) << 8) + VESC_CAN_ID;

CAN_device_t CAN_cfg;

CanBus::CanBus() {}

void CanBus::init() {
    CAN_cfg.speed=CAN_SPEED_500KBPS;
    //CAN_cfg.tx_pin_id = GPIO_NUM_26;
    //CAN_cfg.rx_pin_id = GPIO_NUM_27;
    CAN_cfg.tx_pin_id = GPIO_NUM_27;
    CAN_cfg.rx_pin_id = GPIO_NUM_26;
    CAN_cfg.rx_queue = xQueueCreate(50,sizeof(CAN_frame_t));
  /*  
    CAN_filter_t p_filter;
    p_filter.FM = Single_Mode;

    p_filter.ACR0 = 0x00;
    p_filter.ACR1 = 0x00;
    p_filter.ACR2 = 0x10;
    p_filter.ACR3 = 0x19;

    p_filter.AMR0 = 0x1F;
    p_filter.AMR1 = 0xFF;
    p_filter.AMR2 = 0xFF;
    p_filter.AMR3 = 0xFF;
    ESP32Can.CANConfigFilter(&p_filter);
  */
    //start CAN Module
    ESP32Can.CANInit();
}

/*
  The VESC has to be configured to send status 1-5 regularly. It is recommenden to reduce the
  interval from 50Hz to something around 1-5Hz, which is absolutely sufficient for this application.
*/
void CanBus::loop() {
/*
    // request realtimedata every second 
    if(millis() - lastRealtimeData > 1000) {
      requestRealtimeData();
      lastRealtimeData = millis();
    }

    if(millis() - lastBalanceData > 1000) {
      //requestBalanceData();
      //ping();
      lastBalanceData = millis();
    }
*/

    CAN_frame_t rx_frame;
    //receive next CAN frame from queue
    while(xQueueReceive(CAN_cfg.rx_queue,&rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE){
      //VESC only uses ext packages, so skip std packages
      if(rx_frame.FIR.B.FF==CAN_frame_ext) {
        if(Logger::getLogLevel() == Logger::VERBOSE) {
          printFrame(rx_frame);
        }
        processFrame(rx_frame);
      }
    }
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    dumpVescValues();
  }
}

void CanBus::requestRealtimeData() {
    Logger::notice(LOG_TAG_CANBUS, "requestRealtimeData");
    CAN_frame_t tx_frame;

    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x07;
    tx_frame.data.u8[0] = ESP_CAN_ID;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x32;
    // mask
    tx_frame.data.u8[3] = 0x00;
    tx_frame.data.u8[4] = 0x00;
    tx_frame.data.u8[5] = B10000001;
    tx_frame.data.u8[6] = B11000011;
    ESP32Can.CANWriteFrame(&tx_frame);
}

void CanBus::requestBalanceData(){
    Logger::notice(LOG_TAG_CANBUS, "requestBalanceData");
    CAN_frame_t tx_frame;

    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x03;
    tx_frame.data.u8[0] = ESP_CAN_ID;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x04;
    ESP32Can.CANWriteFrame(&tx_frame);
}

void CanBus::ping(){
    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID  = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PING) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x01;
    tx_frame.data.u8[0] = ESP_CAN_ID;
    ESP32Can.CANWriteFrame(&tx_frame);
}

void CanBus::printFrame(CAN_frame_t rx_frame) {
  if(rx_frame.FIR.B.RTR==CAN_RTR)
    printf(" RTR from 0x%08x, DLC %d\r\n",rx_frame.MsgID,  rx_frame.FIR.B.DLC);
  else{
    printf(" from 0x%08x, DLC %d\n",rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    for(int i = 0; i < 8; i++){
      printf("%d\t", (uint8_t)rx_frame.data.u8[i]);
    }
    printf("\n");
  }
}

void CanBus::processFrame(CAN_frame_t rx_frame) { 
  String frametype = ""; 
  uint32_t ID = rx_frame.MsgID;
  if(STATUS_1 == ID) {
    frametype = "status1";
    vescData.erpm = readInt32Value(rx_frame, 0);
    vescData.current = readInt16Value(rx_frame, 4) / 10.0;
    vescData.dutyCycle = readInt16Value(rx_frame, 6);
  }
  if(STATUS_2 == ID) {
    frametype = "status2";
    vescData.ampHours = readInt32Value(rx_frame, 0) / 10000.0;
    vescData.ampHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
  }
  if(STATUS_3 == ID) {
    frametype = "status3";
    vescData.wattHours = readInt32Value(rx_frame, 0) / 10000.0;
    vescData.wattHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
  }
  if(STATUS_4 == ID) {
    frametype = "status4";
    vescData.mosfetTemp = readInt16Value(rx_frame, 0) / 10.0;
    vescData.motorTemp =  readInt16Value(rx_frame, 2) / 10.0;
    vescData.totalCurrentIn = readInt16Value(rx_frame, 4)  / 10.0;
    vescData.pidPosition = readInt16Value(rx_frame, 6) / 50.0; 
  }
  if(STATUS_5 == ID) {
    frametype = "status5";
    vescData.tachometer = readInt32Value(rx_frame, 0);
    vescData.inputVoltage = readInt16Value(rx_frame, 4) / 10.0;
  }
  if(FILL_RX_BUFFER == ID) {
    frametype = "fill rx buffer";
  }
  if(PROCESS_RX_BUFFER == ID) {
    frametype = "process rx buffer";
  }
 
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[32];
    snprintf(buf, 32, "processed frame %s", frametype.c_str());
    Logger::verbose(LOG_TAG_CANBUS, buf);
  }
}

void CanBus::dumpVescValues() {
    if(millis() - lastDump < 1000) {
        return;
    }
    int size = 25;
    char val[size];
    std::string bufferString = "";
    bufferString += "dutycycle=";
    snprintf(val, size, "%f", vescData.dutyCycle);
    bufferString += val;
    bufferString += ", erpm=";
    snprintf(val, size, "%f", vescData.erpm);
    bufferString += val;
    bufferString += ", current=";
    snprintf(val, size, "%f", vescData.current);
    bufferString += val;
    bufferString += ", ampHours=";
    snprintf(val, size, "%f", vescData.ampHours);
    bufferString += val;
    bufferString +=  ", ampHoursCharged=";
    snprintf(val, size, "%f", vescData.ampHoursCharged);
    bufferString += val;
    bufferString +=  ", wattHours=";
    snprintf(val, size, "%f", vescData.wattHours);
    bufferString += val;
    bufferString +=  ", wattHoursCharged=";
    snprintf(val, size, "%f", vescData.wattHoursCharged);
    bufferString += val;
    bufferString +=  ", mosfetTemp=";
    snprintf(val, size, "%f", vescData.mosfetTemp);
    bufferString += val;
    bufferString +=  ", motorTemp=";
    snprintf(val, size, "%f", vescData.motorTemp);
    bufferString += val;
    bufferString +=  ", inputVoltage=";
    snprintf(val, size, "%f", vescData.inputVoltage);
    bufferString += val;
    bufferString +=  ", tachometer=";
    snprintf(val, size, "%f", vescData.tachometer);
    bufferString += val;
    Logger::verbose(LOG_TAG_CANBUS, bufferString.c_str());
    lastDump = millis();
}

int32_t CanBus::readInt32Value(CAN_frame_t rx_frame, int startbyte) {
  int32_t intVal =  (
    ((int32_t)rx_frame.data.u8[startbyte] << 24) + 
    ((int32_t)rx_frame.data.u8[startbyte+1] << 16) + 
    ((int32_t)rx_frame.data.u8[startbyte+2] << 8) + 
    ((int32_t)rx_frame.data.u8[startbyte+3]));
  return intVal;
}

int16_t CanBus::readInt16Value(CAN_frame_t rx_frame, int startbyte) {
  int16_t intVal = (
      ((int16_t)rx_frame.data.u8[startbyte] << 8) + 
      ((int16_t)rx_frame.data.u8[startbyte+1]));
  return intVal;
}
#endif //CANBUS_ENABLED