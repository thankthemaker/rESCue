#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include "config.h"
#include "ILedController.h"
#include "AppConfiguration.h"
#include <Adafruit_NeoPixel.h>
#include "CanBus.h"

#ifndef PIN_NEOPIXEL
 #define PIN_NEOPIXEL   5
#endif //PIN_NEOPIXEL

#define LOG_TAG_WS28XX "Ws28xxController"

class Ws28xxController : public ILedController, Adafruit_NeoPixel {
    public:
        Ws28xxController(uint16_t pixels, uint8_t pin, uint8_t type, VescData *vescData);
        void init() override;
        void stop() override;
        void startSequence() override;
        void idleSequence() override;
        void changePattern(Pattern pattern, boolean isForward, boolean repeatPattern) override;
        void update() override;

        // Member Variables:  
        Pattern  activePattern    = PULSE; // which pattern is running
        Direction direction       = FORWARD; // direction to run the pattern
        unsigned long interval    = 0;     // milliseconds between updates
        unsigned long lastUpdate  = 0;     // last update of position
        boolean isStartSequence   = true;
        boolean stopPattern       = false; // is pattern stopped
        boolean blockChange       = false; // block changes of pattern (e.g. start-sequence)
        boolean repeat            = false; // repeat the pattern infinitly
        boolean reverseOnComplete = false; // reverse the pattern onComplete

        uint32_t color1 = 0, color2 = 0; // What colors are in use
        uint16_t totalSteps = 0;     // total number of steps in the pattern
        uint16_t index = 0;          // current step within the pattern
    
        void increment();
        void reverse();
        void rainbowCycle(uint8_t interval, Direction dir = FORWARD);
        void rainbowCycleUpdate();
        void flashLight(uint8_t interval = 80, Direction dir = FORWARD);
        void flashLightUpdate();
        void fadeLight(uint8_t interval = 80, Direction dir = FORWARD);
        void fadeLightUpdate();
        void pulsatingLight(uint8_t timeinterval);
        void pulsatingLightUpdate();
        void theaterChase(uint32_t color1, uint32_t color2, uint8_t interval, Direction dir = FORWARD);
        void theaterChaseUpdate();
        void cylon(uint32_t col1, uint8_t interval);
        void cylonUpdate();
        void slidingLight(uint32_t col1, uint32_t col2, uint16_t timeinterval);
        void slidingLightUpdate();
        void batteryIndicator(uint16_t timeinterval);
        void batteryIndicatorUpdate();
        void onComplete();

    private:
        static uint32_t wheel(byte wheelPos);
        void setLight(boolean forward, int brightness);
        static uint32_t dimColor(uint32_t color, uint8_t width);
        static int calcVal(int value);
        uint8_t pulse = 0;
        uint32_t lastPulse = 0;
        boolean up = false;
        int maxBrightness = MAX_BRIGHTNESS;
        Config config = AppConfiguration::getInstance()->config;
        VescData *vescData;
        void flashLightUpdateAll();
        void flashLightUpdateOddEven();
};
#endif //__LED_CONTROLLER_H__