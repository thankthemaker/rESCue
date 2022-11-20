//
// Created by David Anthony Gey on 20.09.22.
//

#ifndef RESCUE_VESCPARSER_H
#define RESCUE_VESCPARSER_H

#include "VescMessage.h"

class VescParser {
  public:
    static VescMessage parseMessage(uint8_t *buffer);
    static VescCommand* handleCommand(int command, uint8_t *buffer);
};


#endif //RESCUE_VESCPARSER_H
