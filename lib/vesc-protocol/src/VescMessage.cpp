//
// Created by David Anthony Gey on 20.09.22.
//

#include "VescMessage.h"

VescMessageType VescMessage::getType() const {
    return type;
}

void VescMessage::setType(VescMessageType type) {
    VescMessage::type = type;
}

int VescMessage::getLength() const {
    return length;
}

void VescMessage::setLength(int length) {
    VescMessage::length = length;
}

int VescMessage::getCommand() const {
    return command;
}

void VescMessage::setCommand(int command) {
    VescMessage::command = command;
}

const VescCommand* VescMessage::getVescCommand() const {
    return vescCommand;
}

void VescMessage::setVescCommand(VescCommand *vescCommand) {
    VescMessage::vescCommand = vescCommand;
}
