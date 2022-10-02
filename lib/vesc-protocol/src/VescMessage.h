//
// Created by David Anthony Gey on 20.09.22.
//

#ifndef RESCUE_VESCMESSAGE_H
#define RESCUE_VESCMESSAGE_H

#include "VescCommand.h"

enum VescMessageType {
    SHORT=2,
    MIDDLE,
    LONG
};

class VescMessage {
public:
    VescMessageType getType() const;
    void setType(VescMessageType type);
    int getLength() const;
    void setLength(int length);
    int getCommand() const;
    void setCommand(int command);

private:
    VescMessageType type;
    int length;
    int command;
    VescCommand* vescCommand;
public:
    const VescCommand* getVescCommand() const;

    void setVescCommand(VescCommand *vescCommand);
};


#endif //RESCUE_VESCMESSAGE_H
