#include "BleCanProxy.h"

#if defined(CANBUS_ENABLED)
BleCanProxy::BleCanProxy(CanDevice *candevice, Stream *stream, uint8_t vesc_id, uint8_t ble_proxy_can_id) {
    this->candevice = candevice;
    this->stream = stream;
    this->vesc_id = vesc_id;
    this->ble_proxy_can_id = ble_proxy_can_id;
}

void BleCanProxy::proxyIn(std::string in) {
    uint8_t packet_type = (uint8_t) in.at(0);

    if (!longPacket) {
        switch (packet_type) {
            case 2:
                length = (uint8_t) in.at(1);
                command = (uint8_t) in.at(2);
                processing = true;
                if (length > (in.size() - 5)) {
                    longPacket = true;
                }
                break;
            case 3:
                length = ((uint8_t) in.at(1) << 8) + (uint8_t) in.at(2);
                command = (uint8_t) in.at(3);
                longPacket = true;
                processing = true;
                break;
            default:
                return;
        }
        if (Logger::getLogLevel() == Logger::VERBOSE) {
            char buf[64];
            snprintf(buf, 64, "Proxy in, command %d, length %d\n", command, length);
            Logger::verbose(LOG_TAG_BLE_CAN_PROXY, buf);
        }
    }

    if (length <= 6) {
        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 0x02 + length;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0x00;
        tx_frame.data[2] = command;
        for (int i = 3; i < length + 2; i++) {
            tx_frame.data[i] = (uint8_t) in.at(i);
        }
        candevice->sendCanFrame(&tx_frame);
    } else {
        longPackBuffer += in;
        if (length + 5 > longPackBuffer.size()) {
            //Serial.printf("Buffer not full, needed %d, is %d\n", length + 5, longPackBuffer.size());
            return;
        }
        //Serial.printf("Buffer full now processing, needed %d, is %d\n", length +  5, longPackBuffer.size());

        int pos = longPacket ? 3 : 2;
        longPackBuffer = longPackBuffer.substr(pos, longPackBuffer.length()-pos);

        unsigned int end_a = 0;
        for (unsigned int byteNum = 0; byteNum < length; byteNum += 7) {
            if (byteNum > 255) {
                break;
            }

            end_a = byteNum + 7;

            int sendLen = (length >= byteNum+ 7) ? 7 : length - byteNum;
            //Serial.printf("bufLen %d, byteNum %d, sendlen %d\n", length, byteNum , sendLen);

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + vesc_id;
            tx_frame.data_length_code = sendLen + 1;
            tx_frame.data[0] = byteNum; //startbyte counter of frame


            for (int i = 1; i < sendLen+1; i++) {
                //Serial.printf("Reading byte %d, length %d, index %d, sendlen %d\n", byteNum + i, bufLen, i, sendLen);
                tx_frame.data[i] = (uint8_t) longPackBuffer.at(byteNum-1 + i);
            }
            candevice->sendCanFrame(&tx_frame);
        }

        for (unsigned int byteNum = end_a; byteNum<length; byteNum += 6) {
            int sendLen = (length >= byteNum + 6) ? 6 : length - byteNum ;
            //Serial.printf("bufLen %d, byteNum %d, sendlen %d\n", length, byteNum , sendLen);

            twai_message_t tx_frame = {};
            tx_frame.extd = 1;
            tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + vesc_id;
            tx_frame.data_length_code= sendLen + 2;
            tx_frame.data[0] = byteNum >> 8;
            tx_frame.data[1] = byteNum & 0xFF;

            for (int i = 2; i < sendLen+2; i++) {
                //Serial.printf("Reading byte %d, length %d, index %d, sendlen %d\n", byteNum + i, bufLen, i, sendLen);
                tx_frame.data[i] = (uint8_t) longPackBuffer.at(byteNum-2 + i);
            }

            candevice->sendCanFrame(&tx_frame);
        }

        twai_message_t tx_frame = {};
        tx_frame.extd = 1;
        tx_frame.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + vesc_id;
        tx_frame.data_length_code = 6;
        tx_frame.data[0] = ble_proxy_can_id;
        tx_frame.data[1] = 0; // IS THIS CORRECT?????
        tx_frame.data[2] = length >> 8;
        tx_frame.data[3] = length & 0xFF;
        tx_frame.data[4] = longPackBuffer.at(longPackBuffer.size() - 3);
        tx_frame.data[5] = longPackBuffer.at(longPackBuffer.size() - 2);
        candevice->sendCanFrame(&tx_frame);

    }

    length = 0;
    command = 0;
    processing = false;
    longPacket = false;
    longPackBuffer = "";
}

void BleCanProxy::proxyOut(uint8_t *data, unsigned int size, uint8_t crc1, uint8_t crc2) {
    if (size > BUFFER_SIZE) {
        Logger::error(LOG_TAG_BLE_CAN_PROXY, "proxyOut - Buffer size exceeded, abort (message not sent via proxy)");
        return;
    }
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[32];
        snprintf(buf, 32, "Proxy out, sending %d bytes\n", size);
        Logger::verbose(LOG_TAG_BLE_CAN_PROXY, buf);
    }
    //Start bit, package size
    if (size <= 255) {
        //Serial.print(0x02);
        stream->write(0x02);
        // size
        //Serial.print(size);
        stream->write(size);
    } else if (size <= 65535) {
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
    for (int i = 0; i < size; i++) {
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
#endif