#ifndef __COB_CONTROLLER_H__
#define __COB_CONTROLLER_H__

#include "config.h"
#include "ILedController.h"

#ifndef MOSFET_PIN_1
 #define MOSFET_PIN_1 22
#endif //MOSFET_PIN_1

#ifndef MOSFET_PIN_2
 #define MOSFET_PIN_2  23
#endif //MOSFET_PIN_2

#define COB_DELAY 50
#define LOG_TAG_COB "CobController"

class CobController : public ILedController {
    public:
      CobController();
        void init();
        void fade(boolean isForward);
        void flash(boolean isForward);
        void stop();
        void startSequence();
        void idleSequence();
        void changePattern(Pattern pattern, boolean isForward, boolean repeatPattern);
        void update();

    private:
      void writePWM(int channel, int dutyCycle);
};

#endif //__COB_CONTROLLER_H__