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

#endif //__COB_CONTROLLER_H__