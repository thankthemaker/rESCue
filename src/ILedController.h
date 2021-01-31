#ifndef __I_LED_CONTROLLER_H__
#define __I_LED_CONTROLLER_H__

#include <Arduino.h>

class ILedController {
    public:    
        // pure virtual (abstract) method definitions
        virtual void init() = 0;
        virtual void fade(int* isForward) = 0;
        virtual void flash(int* isForward) = 0;
        virtual void stop() = 0;
        virtual void startSequence() = 0;
        void loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward);
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

