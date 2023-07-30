#ifndef __I_LED_CONTROLLER_H__
#define __I_LED_CONTROLLER_H__

#include <Arduino.h>
#include "CanBus.h"
#include "VescData.h"

#define LOG_TAG_LED "ILedController"

// Pattern types supported:
enum  Pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, CYLON, FADE, RESCUE_FLASH_LIGHT, PULSE, SLIDE, BATTERY_INDICATOR};
// Patern directions supported:
enum  Direction { FORWARD, REVERSE };

class ILedController {
    public:    
        // pure virtual (abstract) method definitions
        virtual void init() = 0;
        virtual void stop() = 0;
        virtual void idleSequence() = 0;
        virtual void startSequence() = 0;
        virtual void changePattern(Pattern pattern, boolean isForward, boolean repeatPattern ) = 0;
        virtual void update() = 0;
        void loop(const int* new_forward, const int* new_backward, const int* idle, const int* new_brake);

    private:
      const static int bufSize = 128;
      char buf[bufSize];
      int old_forward  = LOW;
      int old_backward = LOW;
      int old_idle     = LOW;
};

class LedControllerFactory {
    public:
        static LedControllerFactory* getInstance();
        static uint8_t determineLedType();
        static ILedController* createLedController(VescData *vescData);

    private:
        LedControllerFactory();    
        static LedControllerFactory *instance;
};

#endif //__I_LED_CONTROLLER_H__

