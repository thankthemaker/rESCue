#ifndef __I_LED_CONTROLLER_H__
#define __I_LED_CONTROLLER_H__

#include <Arduino.h>

#define LOG_TAG_LED "ILedController"

// Pattern types supported:
enum  Pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, CYLON, FADE, RESCUE_FLASH_LIGHT, PULSE, SLIDE};
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
        void loop(int* new_forward, int* new_backward, int* idle);

    private:
      int old_forward  = LOW;
      int old_backward = LOW;
      int old_idle     = LOW;
};

class LedControllerFactory {
    public:
        static LedControllerFactory* getInstance();
        ILedController* createLedController();

    private:
        LedControllerFactory();    
        static LedControllerFactory *instance;    
};

#endif //__I_LED_CONTROLLER_H__

