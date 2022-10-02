//
// Created by David Anthony Gey on 20.09.22.
//

#include <cstdint>
#include "VescParser.h"
#include "VescCommand.h"

VescMessage VescParser::parseMessage(uint8_t *buffer) {
    int index=0, byte;
    VescMessage message;
    byte = buffer[index++];
    if (byte == 0x2) {
        message.setType(VescMessageType::SHORT);
        byte = buffer[index++];
        message.setLength(byte);
    } else {
        message.setType(VescMessageType::MIDDLE);
        byte = buffer[index++];
        message.setLength(byte);
    }
    byte = buffer[index++];
    message.setCommand(byte);
    VescCommand* vescCommand = handleCommand(byte, buffer);
    message.setVescCommand(vescCommand);
    return message;
}

VescCommand* VescParser::handleCommand(int command, uint8_t *buffer) {
    VescCommand* vescCommand = new VescCommand(command);
    return vescCommand;
}