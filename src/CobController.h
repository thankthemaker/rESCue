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

static const char* LOG_TAG_COB = "CobController";

class CobController : public ILedController {
    public:
      CobController();
        void init() override;
        void changePattern(Pattern pattern, boolean isForward, boolean repeatPattern) override;
        void update() override;
        void stop() override;
        void startSequence() override;
        void idleSequence() override;

    private:
      void fade();
      void flash();
      void increment();
      void onComplete();
      void reverse();
      static void writePWM(int channel, int dutyCycle);
      unsigned long interval    = 0;     // milliseconds between updates
      unsigned long lastUpdate  = 0;     // last update of position
      boolean stopPattern       = false; // is pattern stopped
      boolean repeat            = false; // repeat the pattern infinitly
      boolean reverseOnComplete = false; // reverse the pattern onComplete
      Pattern activePattern     = FADE;
      Direction direction       = FORWARD;
      uint16_t totalSteps       = 0;     // total number of steps in the pattern
      uint16_t index            = 0;     // current step within the pattern
};

#endif //__COB_CONTROLLER_H__