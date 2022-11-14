#include "CanBus.h"

#ifdef CANBUS_ENABLED


CanBus::CanBus(VescData *vescData) {
    this->vescData = vescData;
    this->stream = new LoopbackStream(BUFFER_SIZE);
}

void CanBus::init() {
    vesc_id = AppConfiguration::getInstance()->config.vescId;
    esp_can_id = vesc_id + 1;
    ble_proxy_can_id = vesc_id + 2;
    RECV_STATUS_1 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS) << 8) + this->vesc_id;
    RECV_STATUS_2 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_2) << 8) + this->vesc_id;
    RECV_STATUS_3 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_3) << 8) + this->vesc_id;
    RECV_STATUS_4 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_4) << 8) + vesc_id;
    RECV_STATUS_5 = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_5) << 8) + vesc_id;

    RECV_FILL_RX_BUFFER = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + esp_can_id;
    RECV_PROCESS_RX_BUFFER = (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + esp_can_id;

    RECV_PROCESS_SHORT_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + ble_proxy_can_id;
    RECV_FILL_RX_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + ble_proxy_can_id;
    RECV_FILL_RX_BUFFER_LONG_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + ble_proxy_can_id;
    RECV_PROCESS_RX_BUFFER_PROXY =
            (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + ble_proxy_can_id;
    candevice = new CanDevice();
    candevice->init();
    proxy = new BleCanProxy(candevice, stream, vesc_id, ble_proxy_can_id);
}

/*
  The VESC has to be configured to send status 1-5 regularly. It is recommenden to reduce the
  interval from 50Hz to something around 1-5Hz, which is absolutely sufficient for this application.
*/
void CanBus::loop() {
    int frameCount = 0;
    twai_message_t rx_frame;
    if (initialized) {
        if (millis() - lastRealtimeData > interval && !proxy->processing) {
            requestRealtimeData();
        }

        if (millis() - lastBalanceData > interval && !proxy->processing) {
            requestBalanceData();
        }
    } else if(initRetryCounter > 0 && millis() - lastRetry > 500) {
        requestFirmwareVersion();
        initRetryCounter--;
        lastRetry = millis();
        if(initRetryCounter == 0) {
            Logger::error("CANBUS initialization failed");
        }
    }

    //receive next CAN frame from queue
    while (twai_receive( &rx_frame, 3 * portTICK_PERIOD_MS) == ESP_OK) {
        if (!initialized) {
            Logger::notice(LOG_TAG_CANBUS, "CANBUS is now initialized");
            initialized = true;
        }
        frameCount++;
        //VESC only uses ext packages, so skip std packages
        if (rx_frame.extd) {
            if (Logger::getLogLevel() == Logger::VERBOSE) {
                printFrame(rx_frame, frameCount);
            }
            processFrame(rx_frame, frameCount);
        }
        clearFrame(rx_frame);
        if (frameCount > 1000) {
            // WORKAROUND if messages arrive too fast
            Logger::error(LOG_TAG_CANBUS, "reached 1000 frames in one loop, abort");
            buffer.clear();
            return;
        }
    }
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        dumpVescValues();
    }
}

void CanBus::requestFirmwareVersion() {
    Logger::notice(LOG_TAG_CANBUS, "requestFirmwareVersion");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x03;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x00;  // COMM_FW_VERSION
    candevice->sendCanFrame(&tx_frame);
}

void CanBus::requestRealtimeData() {
    Logger::notice(LOG_TAG_CANBUS, "requestRealtimeData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x07;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x32;      // COMM_GET_VALUES_SELECTIVE
    // mask
    tx_frame.data[3] = 0x00;      // Byte1 of mask (Bits 24-31)
    tx_frame.data[4] = 0x00;      // Byte2 of mask (Bits 16-23)
    tx_frame.data[5] = B10000111; // Byte3 of mask (Bits 8-15)
    tx_frame.data[6] = B11000011; // Byte4 of mask (Bits 0-7)
    candevice->sendCanFrame(&tx_frame);
}

void CanBus::requestBalanceData() {
    Logger::notice(LOG_TAG_CANBUS, "requestBalanceData");
    twai_message_t tx_frame = {};

    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
    tx_frame.data_length_code = 0x03;
    tx_frame.data[0] = esp_can_id;
    tx_frame.data[1] = 0x00;
    tx_frame.data[2] = 0x4F;  // COMM_GET_DECODED_BALANCE
    candevice->sendCanFrame(&tx_frame);
}

void CanBus::ping() {
    twai_message_t tx_frame = {};
    tx_frame.extd = 1;
    tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PING) << 8) + vesc_id;
    tx_frame.data_length_code = 0x01;
    tx_frame.data[0] = esp_can_id;
    candevice->sendCanFrame(&tx_frame);
}

void CanBus::printFrame(twai_message_t rx_frame, int frameCount) {
    if (rx_frame.rtr)
        printf("#%d RTR from 0x%08x, DLC %d\r\n", frameCount, rx_frame.identifier, rx_frame.data_length_code);
    else {
        printf("#%d from 0x%08x, DLC %d, data [", frameCount, rx_frame.identifier, rx_frame.data_length_code);
        for (int i = 0; i < 8; i++) {
            printf("%d", (uint8_t) rx_frame.data[i]);
            if (i != 7) {
                printf("\t");
            }
        }
        printf("]\n");
    }
}

void CanBus::clearFrame(twai_message_t rx_frame) {
    rx_frame.identifier=0;
    rx_frame.data_length_code=0;
    rx_frame.data[0]=0;
    rx_frame.data[1]=0;
    rx_frame.data[2]=0;
    rx_frame.data[3]=0;
    rx_frame.data[4]=0;
    rx_frame.data[5]=0;
    rx_frame.data[6]=0;
    rx_frame.data[7]=0;
}

void CanBus::processFrame(twai_message_t rx_frame, int frameCount) {
    String frametype = "";
    uint32_t ID = rx_frame.identifier;
    if (RECV_STATUS_1 == ID) {
        frametype = "status1";
        vescData->erpm = readInt32Value(rx_frame, 0);
        vescData->current = readInt16Value(rx_frame, 4) / 10.0;
        vescData->dutyCycle = readInt16Value(rx_frame, 6);
    }
    if (RECV_STATUS_2 == ID) {
        frametype = "status2";
        vescData->ampHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->ampHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    }
    if (RECV_STATUS_3 == ID) {
        frametype = "status3";
        vescData->wattHours = readInt32Value(rx_frame, 0) / 10000.0;
        vescData->wattHoursCharged = readInt32Value(rx_frame, 4) / 10000.0;
    }
    if (RECV_STATUS_4 == ID) {
        frametype = "status4";
        vescData->mosfetTemp = readInt16Value(rx_frame, 0) / 10.0;
        vescData->motorTemp = readInt16Value(rx_frame, 2) / 10.0;
        vescData->totalCurrentIn = readInt16Value(rx_frame, 4) / 10.0;
        vescData->pidPosition = readInt16Value(rx_frame, 6) / 50.0;
    }
    if (RECV_STATUS_5 == ID) {
        frametype = "status5";
        vescData->tachometer = readInt32Value(rx_frame, 0);
        vescData->inputVoltage = readInt16Value(rx_frame, 4) / 10.0;
        vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
    }

    if (RECV_PROCESS_SHORT_BUFFER_PROXY == ID) {
        frametype = "process short buffer for <<BLE proxy>>";
        for (int i = 1; i < rx_frame.data_length_code; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
        proxy->proxyOut(proxybuffer.data(), proxybuffer.size(), rx_frame.data[4], rx_frame.data[5]);
        proxybuffer.clear();
    }

    if (RECV_FILL_RX_BUFFER == ID) {
        frametype = "fill rx buffer";
        for (int i = 1; i < rx_frame.data_length_code; i++) {
            buffer.push_back(rx_frame.data[i]);
        }
    }

    if (RECV_FILL_RX_BUFFER_PROXY == ID || RECV_FILL_RX_BUFFER_LONG_PROXY == ID) {
        boolean longBuffer = RECV_FILL_RX_BUFFER_LONG_PROXY == ID;
        frametype = longBuffer ? "fill rx long buffer" : "fill rx buffer";
        for (int i = (longBuffer ? 2 : 1); i < rx_frame.data_length_code; i++) {
            proxybuffer.push_back(rx_frame.data[i]);
        }
    }

    if (RECV_PROCESS_RX_BUFFER == ID || RECV_PROCESS_RX_BUFFER_PROXY == ID) {
        boolean isProxyRequest = false;
        frametype = "process rx buffer for ";
        if (RECV_PROCESS_RX_BUFFER_PROXY == ID) {
            frametype += " <<BLE proxy>> ";
            isProxyRequest = true;
        }
        //Serial.printf("bytes %d\n", buffer.size());

        if ((!isProxyRequest && buffer.empty()) || (isProxyRequest && proxybuffer.empty())) {
            Serial.printf("buffer empty, abort");
            return;
        }
        uint8_t command = (isProxyRequest ? proxybuffer : buffer).at(0);
        if (command == 0x00) {
            frametype += "COMM_FW_VERSION";
            int offset = 1;
            vescData->majorVersion = readInt8ValueFromBuffer(0, isProxyRequest);
            vescData->minorVersion = readInt8ValueFromBuffer(1, isProxyRequest);
            vescData->name = readStringValueFromBuffer(2 + offset, 12, isProxyRequest);
        } else if (command == 0x4F) {  //0x4F = 79 DEC
            frametype += "COMM_GET_DECODED_BALANCE";
            int offset = 1;
            vescData->pidOutput = readInt32ValueFromBuffer(0 + offset, isProxyRequest) / 1000000.0;
            vescData->pitch = readInt32ValueFromBuffer(4 + offset, isProxyRequest) / 1000000.0;
            vescData->roll = readInt32ValueFromBuffer(8 + offset, isProxyRequest) / 1000000.0;
            vescData->loopTime = readInt32ValueFromBuffer(12 + offset, isProxyRequest);
            vescData->motorCurrent = readInt32ValueFromBuffer(16 + offset, isProxyRequest) / 1000000.0;
            vescData->motorPosition = readInt32ValueFromBuffer(20 + offset, isProxyRequest) / 1000000.0;
            vescData->balanceState = readInt16ValueFromBuffer(24 + offset, isProxyRequest);
            vescData->switchState = readInt16ValueFromBuffer(26 + offset, isProxyRequest);
            vescData->adc1 = readInt32ValueFromBuffer(28 + offset, isProxyRequest) / 1000000.0;
            vescData->adc2 = readInt32ValueFromBuffer(32 + offset, isProxyRequest) / 1000000.0;
            lastBalanceData = millis();
        } else if (command == 0x32) { //0x32 = 50 DEC
            frametype += "COMM_GET_VALUES_SELECTIVE";
            int offset = 1;
            vescData->mosfetTemp = readInt16ValueFromBuffer(4 + offset, isProxyRequest) / 10.0;
            vescData->motorTemp = readInt16ValueFromBuffer(6 + offset, isProxyRequest) / 10.0;
            vescData->dutyCycle = readInt16ValueFromBuffer(8 + offset, isProxyRequest) / 1000.0;
            vescData->erpm = readInt32ValueFromBuffer(10 + offset, isProxyRequest);
            vescData->inputVoltage = readInt16ValueFromBuffer(14 + offset, isProxyRequest) / 10.0;
            vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
            vescData->tachometer = readInt32ValueFromBuffer(16 + offset, isProxyRequest);
            vescData->tachometerAbsolut = readInt32ValueFromBuffer(20 + offset, isProxyRequest);
            vescData->fault = readInt8ValueFromBuffer(24 + offset, isProxyRequest);
            lastRealtimeData = millis();
        }  else if (command == 0x33) { //0x33 = 51 DEC
            frametype += "COMM_GET_VALUES_SETUP_SELECTIVE";
            int offset = 1;
            int startbyte = 0;
            uint32_t bitmask = readInt32ValueFromBuffer(0 + offset, isProxyRequest);
            startbyte += 4;
            if(bitmask & ((uint32_t) 1 << 0)) {
                vescData->mosfetTemp = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 1)) {
                vescData->motorTemp = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 2)) {
                // current in
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 3)) {
                // current in_total
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 4)) {
                vescData->dutyCycle = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 1000.0;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 5)) {
                vescData->erpm = readInt32ValueFromBuffer(startbyte + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 6)) {
                // speed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 7)) {
                vescData->inputVoltage = readInt16ValueFromBuffer(startbyte + offset, isProxyRequest) / 10.0;
                vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 8)) {
                // battery level
                startbyte += 2;
            }
            if(bitmask & ((uint32_t) 1 << 9)) {
                // amphours consumed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 10)) {
                // amphours charged
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 11)) {
                // watthours consumed
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 12)) {
                // watthours charged
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 13)) {
                vescData->tachometer = readInt32ValueFromBuffer(16 + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 14)) {
                vescData->tachometerAbsolut = readInt32ValueFromBuffer(20 + offset, isProxyRequest);
                startbyte += 4;
            }
            if(bitmask & ((uint32_t) 1 << 16)) {
                vescData->fault = readInt8ValueFromBuffer(24 + offset, isProxyRequest);
                startbyte += 1;
            }
            lastRealtimeData = millis();
        } else if (command == 0x04) {
            frametype += "COMM_GET_VALUES";
            int offset = 1;
            vescData->mosfetTemp = readInt16ValueFromBuffer(0 + offset, isProxyRequest) / 10.0;
            vescData->motorTemp = readInt16ValueFromBuffer(2 + offset, isProxyRequest) / 10.0;
            vescData->motorCurrent = readInt32ValueFromBuffer(4 + offset, isProxyRequest) / 100.0;
            vescData->current = readInt32ValueFromBuffer(8 + offset, isProxyRequest) / 100.0;
            // id = vescData->readInt32ValueFromBuffer(12 + offset, isProxyRequest) / 100.0;
            // iq = vescData->readInt32ValueFromBuffer(16 + offset, isProxyRequest) / 100.0;
            vescData->dutyCycle = readInt16ValueFromBuffer(20 + offset, isProxyRequest) / 1000.0;
            vescData->erpm = readInt32ValueFromBuffer(22 + offset, isProxyRequest);
            vescData->inputVoltage = readInt16ValueFromBuffer(26 + offset, isProxyRequest) / 10.0;
            vescData->inputVoltage += AppConfiguration::getInstance()->config.batteryDrift;
            vescData->ampHours =  readInt32ValueFromBuffer(28 + offset, isProxyRequest) / 10000.0;
            vescData->ampHoursCharged = readInt32ValueFromBuffer(32 + offset, isProxyRequest) / 10000.0;
            vescData->wattHours =  readInt32ValueFromBuffer(46 + offset, isProxyRequest) / 10000.0;
            vescData->wattHoursCharged = readInt32ValueFromBuffer(40 + offset, isProxyRequest) / 10000.0;
            vescData->tachometer = readInt32ValueFromBuffer(44 + offset, isProxyRequest);
            vescData->tachometerAbsolut = readInt32ValueFromBuffer(58 + offset, isProxyRequest);
            vescData->fault = readInt8ValueFromBuffer(52 + offset, isProxyRequest);
            lastRealtimeData = millis();
        } else if (command == 0x0E) {  //0x0E = 14 DEC
            frametype += "COMM_GET_MCCONF";
        } else if (command == 0x11) {  //0x11 = 17 DEC
            frametype += "COMM_GET_APPCONF";
        } else if (command == 0x2F) { //0x2F = 47 DEC
            frametype += "COMM_GET_VALUES_SETUP";
        } else if (command == 0x33) { //0x33 = 51 DEC
            frametype += "COMM_GET_VALUES_SETUP_SELECTIVE";
        } else if (command == 0x41) { //0x41 = 65 DEC
            frametype += "COMM_GET_IMU_DATA";
        } else if (command == 0x3E) { //0x3E = 62 DEC
            frametype += "COMM_PING_CAN";
        } else {
            frametype += command;
        }
        if (isProxyRequest) {
            proxy->proxyOut(proxybuffer.data(), proxybuffer.size(), rx_frame.data[4], rx_frame.data[5]);
            proxybuffer.clear();
        } else {
            buffer.clear();
        }
    }

    if (Logger::getLogLevel() <= Logger::NOTICE) {
        char buf[128];
        snprintf(buf, 128, "processed frame #%d, type %s", frameCount, frametype.c_str());
        Logger::notice(LOG_TAG_CANBUS, buf);
    }
}

void CanBus::dumpVescValues() {
    if (Logger::getLogLevel() != Logger::VERBOSE || millis() - lastDump < 1000) {
        return;
    }
    int size = 25;
    char val[size];
    std::string bufferString;
    bufferString += "name=";
    snprintf(val, size, "%s, ", vescData->name.c_str());
    bufferString += val;
    bufferString += "dutycycle=";
    snprintf(val, size, "%f", vescData->dutyCycle);
    bufferString += val;
    bufferString += ", erpm=";
    snprintf(val, size, "%f", vescData->erpm);
    bufferString += val;
    bufferString += ", current=";
    snprintf(val, size, "%f", vescData->current);
    bufferString += val;
    bufferString += ", ampHours=";
    snprintf(val, size, "%f", vescData->ampHours);
    bufferString += val;
    bufferString += ", ampHoursCharged=";
    snprintf(val, size, "%f", vescData->ampHoursCharged);
    bufferString += val;
    bufferString += ", wattHours=";
    snprintf(val, size, "%f", vescData->wattHours);
    bufferString += val;
    bufferString += ", wattHoursCharged=";
    snprintf(val, size, "%f", vescData->wattHoursCharged);
    bufferString += val;
    bufferString += ", mosfetTemp=";
    snprintf(val, size, "%f", vescData->mosfetTemp);
    bufferString += val;
    bufferString += ", motorTemp=";
    snprintf(val, size, "%f", vescData->motorTemp);
    bufferString += val;
    bufferString += ", inputVoltage=";
    snprintf(val, size, "%f", vescData->inputVoltage);
    bufferString += val;
    bufferString += ", tachometer=";
    snprintf(val, size, "%f", vescData->tachometer);
    bufferString += val;
    bufferString += ", pidOutput=";
    snprintf(val, size, "%f", vescData->pidOutput);
    bufferString += val;
    bufferString += ", pitch=";
    snprintf(val, size, "%f", vescData->pitch);
    bufferString += val;
    bufferString += ", roll=";
    snprintf(val, size, "%f", vescData->roll);
    bufferString += val;
    bufferString += ", loopTime=";
    snprintf(val, size, "%d", vescData->loopTime);
    bufferString += val;
    bufferString += ", motorCurrent=";
    snprintf(val, size, "%f", vescData->motorCurrent);
    bufferString += val;
    bufferString += ", motorPosition=";
    snprintf(val, size, "%f", vescData->motorPosition);
    bufferString += val;
    bufferString += ", balanceState=";
    snprintf(val, size, "%d", vescData->balanceState);
    bufferString += val;
    bufferString += ", switchState=";
    snprintf(val, size, "%d", vescData->switchState);
    bufferString += val;
    bufferString += ", adc1=";
    snprintf(val, size, "%f", vescData->adc1);
    bufferString += val;
    bufferString += ", adc2=";
    snprintf(val, size, "%f", vescData->adc2);
    bufferString += val;
    bufferString += ", fault=";
    snprintf(val, size, "%d", vescData->fault);
    bufferString += val;
    Logger::verbose(LOG_TAG_CANBUS, bufferString.c_str());
    lastDump = millis();
}

int32_t CanBus::readInt32Value(twai_message_t rx_frame, int startbyte) {
    int32_t intVal = (
            ((int32_t) rx_frame.data[startbyte] << 24) +
            ((int32_t) rx_frame.data[startbyte + 1] << 16) +
            ((int32_t) rx_frame.data[startbyte + 2] << 8) +
            ((int32_t) rx_frame.data[startbyte + 3]));
    return intVal;
}

int16_t CanBus::readInt16Value(twai_message_t rx_frame, int startbyte) {
    int16_t intVal = (
            ((int16_t) rx_frame.data[startbyte] << 8) +
            ((int16_t) rx_frame.data[startbyte + 1]));
    return intVal;
}

int32_t CanBus::readInt32ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    int32_t intVal = (
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte) << 24) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 1) << 16) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 2) << 8) +
            ((int32_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 3)));
    return intVal;
}

int16_t CanBus::readInt16ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    int16_t intVal = (
            ((int16_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte) << 8) +
            ((int16_t) (isProxyRequest ? proxybuffer : buffer).at(startbyte + 1)));
    return intVal;
}

int8_t CanBus::readInt8ValueFromBuffer(int startbyte, boolean isProxyRequest) {
    return (isProxyRequest ? proxybuffer : buffer).at(startbyte);
}

std::string CanBus::readStringValueFromBuffer(int startbyte, int length, boolean isProxyRequest) {
    std::string name;
    for (int i = startbyte; i < startbyte + length; i++) {
        name += (char) (isProxyRequest ? proxybuffer : buffer).at(i);
    }
    return name;
}

#endif //CANBUS_ENABLED