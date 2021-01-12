#ifndef COB_CONTROLLER_H
#define COB_CONTROLLER_H

#include "config.h"
#include "ILedController.h"

#ifndef MOSFET_PIN_1
 #define MOSFET_PIN_1 22
#endif

#ifndef MOSFET_PIN_2
 #define MOSFET_PIN_2  23
#endif

#define COB_DELAY 50

class CobController : public ILedController {
    public:
      CobController();
        void init();
        void fadeIn(boolean isForward);
        void fadeOut(boolean isForward);
        void flash(boolean isForward);
        void stop();
        void startSequence(byte red, byte green, byte blue, int speedDelay);
        void loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward);
    private:
      void writePWM(int channel, int dutyCycle);
};

#endif