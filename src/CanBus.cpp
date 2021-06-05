#include "CanBus.h"
#include <Logger.h>

#ifdef CANBUS_ENABLED

#ifndef VESC_CAN_ID
 #define VESC_CAN_ID 25
#endif
#ifndef ESP_CAN_ID
 #define ESP_CAN_ID 3
#endif

#define BUFFER_SIZE 1024

int lastDump = 0;
int lastStatus = 0;
int lastRealtimeData = 0;
int lastBalanceData = 0;
std::vector<uint8_t> buffer = {};
uint32_t STATUS_1          = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS)            << 8) + VESC_CAN_ID;
uint32_t STATUS_2          = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_2)          << 8) + VESC_CAN_ID;
uint32_t STATUS_3          = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_3)          << 8) + VESC_CAN_ID;
uint32_t STATUS_4          = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_4)          << 8) + VESC_CAN_ID;
uint32_t STATUS_5          = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_STATUS_5)          << 8) + VESC_CAN_ID;
uint32_t FILL_RX_BUFFER    = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_FILL_RX_BUFFER)    << 8) + VESC_CAN_ID;
uint32_t PROCESS_RX_BUFFER = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_PROCESS_RX_BUFFER) << 8) + VESC_CAN_ID;

uint32_t RECV_FILL_RX_BUFFER    = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_FILL_RX_BUFFER)    << 8) + ESP_CAN_ID;
uint32_t RECV_PROCESS_RX_BUFFER = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_PROCESS_RX_BUFFER) << 8) + ESP_CAN_ID;

uint32_t RECV_PROCESS_SHORT_BUFFER_PROXY = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + BLE_CAN_PROXY_ID;
uint32_t RECV_FILL_RX_BUFFER_PROXY       = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_FILL_RX_BUFFER)       << 8) + BLE_CAN_PROXY_ID;
uint32_t RECV_FILL_RX_BUFFER_LONG_PROXY  = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_FILL_RX_BUFFER_LONG)  << 8) + BLE_CAN_PROXY_ID;
uint32_t RECV_PROCESS_RX_BUFFER_PROXY    = (uint32_t (0x0000) << 16) + (uint16_t (CAN_PACKET_PROCESS_RX_BUFFER)    << 8) + BLE_CAN_PROXY_ID;

CAN_device_t CAN_cfg;

CanBus::CanBus() {
  stream = new LoopbackStream(BUFFER_SIZE);
}

void CanBus::init() {
    CAN_cfg.speed=CAN_SPEED_500KBPS;
    CAN_cfg.tx_pin_id = GPIO_NUM_26;
    CAN_cfg.rx_pin_id = GPIO_NUM_27;
    CAN_cfg.rx_queue = xQueueCreate(1000, sizeof(CAN_frame_t));
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
    requestFirmwareVersion();
}

/*
  The VESC has to be configured to send status 1-5 regularly. It is recommenden to reduce the
  interval from 50Hz to something around 1-5Hz, which is absolutely sufficient for this application.
*/
void CanBus::loop() {
/*
     request realtimedata every second 
    if(millis() - lastRealtimeData > 1000) {
      requestRealtimeData();
      lastRealtimeData = millis();
    }
*/
/*
    if(millis() - lastBalanceData > 10000) {
      requestBalanceData();
      lastBalanceData = millis();
    }
*/
    int frameCount = 0;
    CAN_frame_t rx_frame;
    //receive next CAN frame from queue
    while(xQueueReceive(CAN_cfg.rx_queue,&rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE){
      //VESC only uses ext packages, so skip std packages
      if(rx_frame.FIR.B.FF==CAN_frame_ext) {
        if(Logger::getLogLevel() == Logger::VERBOSE) {
          printFrame(rx_frame, frameCount++);
        }
        processFrame(rx_frame);
      }
    }
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    dumpVescValues();
  }
}

void CanBus::requestFirmwareVersion() {
    Logger::notice(LOG_TAG_CANBUS, "requestFirmwareVersion");
    CAN_frame_t tx_frame = {};

    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x03;
    tx_frame.data.u8[0] = BLE_CAN_PROXY_ID;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x00;
    sendCanFrame(&tx_frame);
}

void CanBus::requestRealtimeData() {
    Logger::notice(LOG_TAG_CANBUS, "requestRealtimeData");
    CAN_frame_t tx_frame = {};

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
    sendCanFrame(&tx_frame);
}

void CanBus::requestBalanceData(){
    Logger::notice(LOG_TAG_CANBUS, "requestBalanceData");
    CAN_frame_t tx_frame = {};

    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x03;
    tx_frame.data.u8[0] = ESP_CAN_ID;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x4F;
    sendCanFrame(&tx_frame);
}

void CanBus::ping(){
    CAN_frame_t tx_frame = {};
    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID  = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PING) << 8) + VESC_CAN_ID;
    tx_frame.FIR.B.DLC = 0x01;
    tx_frame.data.u8[0] = ESP_CAN_ID;
    sendCanFrame(&tx_frame);
}

void CanBus::printFrame(CAN_frame_t rx_frame, int frameCount) {
  if(rx_frame.FIR.B.RTR==CAN_RTR)
    printf("#%d RTR from 0x%08x, DLC %d\r\n", frameCount, rx_frame.MsgID, rx_frame.FIR.B.DLC);
  else{
    printf("#%d from 0x%08x, DLC %d, data [", frameCount, rx_frame.MsgID, rx_frame.FIR.B.DLC);
    for(int i = 0; i < 8; i++){
      printf("%d", (uint8_t)rx_frame.data.u8[i]);
      if(i != 7) {
        printf("\t");
      }
    }
    printf("]\n");
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

  if(RECV_PROCESS_SHORT_BUFFER_PROXY == ID) {
    frametype = "process short buffer for <<BLE proxy>>";
    for(int i=1; i<rx_frame.FIR.B.DLC; i++) {
      buffer.push_back(rx_frame.data.u8[i]);
    }
    proxyOut(buffer.data(), buffer.size(), rx_frame.data.u8[4], rx_frame.data.u8[5]);
    buffer.clear();
  }

  if(RECV_FILL_RX_BUFFER == ID || RECV_FILL_RX_BUFFER_PROXY == ID || RECV_FILL_RX_BUFFER_LONG_PROXY == ID) {
    frametype = "fill rx buffer";
    boolean longBuffer = RECV_FILL_RX_BUFFER_LONG_PROXY == ID;
    for(int i=(longBuffer ? 2 : 1); i<rx_frame.FIR.B.DLC; i++) {
      buffer.push_back(rx_frame.data.u8[i]);
    }
  }

  if(RECV_PROCESS_RX_BUFFER_PROXY == ID) {
    frametype = "process rx buffer for <<BLE proxy>>";
    proxyOut(buffer.data(), buffer.size(), rx_frame.data.u8[4], rx_frame.data.u8[5]);
    buffer.clear();
  }

  if(RECV_PROCESS_RX_BUFFER == ID) {
    frametype = "process rx buffer for ";
    Serial.printf("bytes %d\n", buffer.size());

    if(buffer.size() <= 0) {
      Serial.printf("buffer empty, abort");
      return;
    }
    if(buffer.at(1) == 0x00) {
      frametype += "firmwareversion";
      int offset = 2;
      vescData.majorVersion = readInt8ValueFromBuffer(0);
      vescData.minorVersion = readInt8ValueFromBuffer(1);
      vescData.name = readStringValueFromBuffer(2 + offset, 12);
    }
    if(buffer.at(1) == 0x4F) {
      frametype += "balancedata";
      int offset = 2;
      vescData.pidOutput = readInt32ValueFromBuffer(0 + offset) / 1000000.0;
      vescData.pitch = readInt32ValueFromBuffer(4 + offset) / 1000000.0;
      vescData.roll = readInt32ValueFromBuffer(8 + offset) / 1000000.0;
      vescData.loopTime = readInt32ValueFromBuffer(12 + offset);
      vescData.motorCurrent = readInt32ValueFromBuffer(16 + offset) / 1000000.0;
      vescData.motorPosition = readInt32ValueFromBuffer(20 + offset) / 1000000.0;
      vescData.balanceState = readInt16ValueFromBuffer(24 + offset);
      vescData.switchState = readInt16ValueFromBuffer(26 + offset);
      vescData.adc1 = readInt32ValueFromBuffer(28 + offset)  / 1000000.0;
      vescData.adc2 = readInt32ValueFromBuffer(32 + offset)  / 1000000.0;
    }

    if(buffer.at(1) == 0x32) {
      frametype += "realtimeData";
      int offset = 2;
      vescData.mosfetTemp = readInt16ValueFromBuffer(4 + offset) / 10.0;
      vescData.motorTemp = readInt16ValueFromBuffer(6) / 10.0;
      vescData.dutyCycle = readInt16ValueFromBuffer(8) / 1000.0;
      vescData.erpm = readInt16ValueFromBuffer(10);
      vescData.inputVoltage = readInt16ValueFromBuffer(14) / 10.0;
      vescData.fault = readInt8ValueFromBuffer(16);
    }
    proxyOut(buffer.data(), buffer.size(), rx_frame.data.u8[4], rx_frame.data.u8[5]);
    buffer.clear();
  }
 
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[128];
    snprintf(buf, 128, "processed frame %s", frametype.c_str());
    Logger::verbose(LOG_TAG_CANBUS, buf);
  }
}

void CanBus::proxyIn(std::string in) {
  uint8_t length = (uint8_t) in.substr(1, 1).at(0);
  uint8_t command = (uint8_t) in.substr(2, 1).at(0);
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "Proxy in, command %d, length %d\n", command, length);
    Logger::verbose(LOG_TAG_CANBUS, buf);
  }
  
  if(length > 6) {
    Logger::warning(LOG_TAG_CANBUS, "Packets bigger 6 byte are not supported yet, aborting.");
  } 

  CAN_frame_t tx_frame = {};

  tx_frame.FIR.B.FF = CAN_frame_ext;
  tx_frame.MsgID = (uint32_t (0x8000) << 16) + (uint16_t (CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_CAN_ID;
  tx_frame.FIR.B.DLC = 0x02 + length;
  tx_frame.data.u8[0] = BLE_CAN_PROXY_ID;
  tx_frame.data.u8[1] = 0x00;
  tx_frame.data.u8[2] = command;
  for(int i=3; i<length+2;i++) {
    tx_frame.data.u8[i] = (uint8_t) in.substr(i, 1).at(0);
  }
  sendCanFrame(&tx_frame);
}

void CanBus::proxyOut(uint8_t *data, int size, uint8_t crc1, uint8_t crc2) {
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[32];
    snprintf(buf, 32, "Proxy out, sending %d bytes\n", size);
    Logger::verbose(LOG_TAG_CANBUS, buf);
  }
  //Start bit, package size
  if(size <= 255) {
    //Serial.print(0x02);
    stream->write(0x02);
    // size
    //Serial.print(size);
    stream->write(size);
  } else if(size <= 65535) {
    //Serial.print(0x03);
    stream->write(0x03);
    // size
    //Serial.print(size >> 8);
    //Serial.print(size & 0xFF);
    stream->write(size >> 8);
    stream->write(size & 0xFF);
  } else {
    //Serial.print(0x04);
    stream->write(0x04);
    // size
    //Serial.print(size >> 16);
    //Serial.print((size >> 8) & 0x0F);
    //Serial.print(size & 0xFF);
    stream->write(size >> 16);
    stream->write((size >> 8) & 0x0F);
    stream->write(size & 0xFF);
 }

  // data
  for(int i=0; i<size; i++){
    //Serial.print(data[i]);
    stream->write(data[i]);
  } 

  //crc 2 byte
  //Serial.print(crc1);
  //Serial.print(crc2);  
  stream->write(crc1);
  stream->write(crc2);

  // Stop bit
  //Serial.print(0x03);
  stream->write(0x03);

  //Serial.println("");
}

void CanBus::dumpVescValues() {
    if(millis() - lastDump < 1000) {
        return;
    }
    int size = 25;
    char val[size];
    std::string bufferString = "";
    if(Logger::getLogLevel() == Logger::VERBOSE) {
      bufferString += "name=";
      snprintf(val, size, "%s, ", vescData.name.c_str());
      bufferString += val;
    }
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
    bufferString +=  ", pidOutput=";
    snprintf(val, size, "%f", vescData.pidOutput);
    bufferString += val;
    bufferString +=  ", pitch=";
    snprintf(val, size, "%f", vescData.pitch);
    bufferString += val;
    bufferString +=  ", roll=";
    snprintf(val, size, "%f", vescData.roll);
    bufferString += val;
    bufferString +=  ", loopTime=";
    snprintf(val, size, "%d", vescData.loopTime);
    bufferString += val;
    bufferString +=  ", motorCurrent=";
    snprintf(val, size, "%f", vescData.motorCurrent);
    bufferString += val;
    bufferString +=  ", motorPosition=";
    snprintf(val, size, "%f", vescData.motorPosition);
    bufferString += val;
    bufferString +=  ", balanceState=";
    snprintf(val, size, "%d", vescData.balanceState);
    bufferString += val;
    bufferString +=  ", switchState=";
    snprintf(val, size, "%d", vescData.balanceState);
    bufferString += val;
    bufferString +=  ", adc1=";
    snprintf(val, size, "%f", vescData.adc1);
    bufferString += val;
    bufferString +=  ", adc2=";
    snprintf(val, size, "%f", vescData.adc2);
    bufferString += val;    
    bufferString +=  ", fault=";
    snprintf(val, size, "%d", vescData.fault);
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

int32_t CanBus::readInt32ValueFromBuffer(int startbyte) {
  int32_t intVal =  (
    ((int32_t)buffer.at(startbyte) << 24) + 
    ((int32_t)buffer.at(startbyte+1) << 16) + 
    ((int32_t)buffer.at(startbyte+2) << 8) + 
    ((int32_t)buffer.at(startbyte+3)));
  return intVal;
}

int16_t CanBus::readInt16ValueFromBuffer(int startbyte) {
  int16_t intVal = (
      ((int16_t)buffer.at(startbyte) << 8) + 
      ((int16_t)buffer.at(startbyte+1)));
  return intVal;
}

int8_t CanBus::readInt8ValueFromBuffer(int startbyte) {
  return buffer.at(startbyte);
}

std::string CanBus::readStringValueFromBuffer(int startbyte, int length) {
  std::string name = "";
  for(int i=startbyte; i<startbyte+length; i++) {
    name += (char) buffer.at(i);
  }
  return name;
}

void CanBus::sendCanFrame(const CAN_frame_t* p_frame) {
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "Sending CAN frame %" PRIu32 ", [%d, %d, %d, %d, %d, %d, %d, %d]\n", 
      p_frame->MsgID, 
      p_frame->data.u8[0],
      p_frame->data.u8[1],
      p_frame->data.u8[2],
      p_frame->data.u8[3],
      p_frame->data.u8[4],
      p_frame->data.u8[5],
      p_frame->data.u8[6],
      p_frame->data.u8[7]);
    Logger::verbose(LOG_TAG_CANBUS, buf);
  }
  ESP32Can.CANWriteFrame(p_frame);
}

#endif //CANBUS_ENABLED