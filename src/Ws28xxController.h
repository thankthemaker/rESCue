#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include "config.h"
#include "ILedController.h"
#include <Adafruit_NeoPixel.h>

#ifndef PIN_NEOPIXEL
 #define PIN_NEOPIXEL   5
#endif //PIN_NEOPIXEL

#ifndef NUMPIXELS
 #define NUMPIXELS    16  
#endif //NUMPIXELS

extern Adafruit_NeoPixel pixels;

class Ws28xxController : public ILedController {
    public:
        Ws28xxController();
        void init();
        void fade(int* isForward);
        void flash(int* isForward);
        void stop();
        void startSequence();
    private:
        void setLight(boolean forward, byte brightness);
        void setPixel(int pixel, byte red, byte green, byte blue);
        void showStrip();
        void startSequenceChasing(byte red, byte green, byte blue, int speedDelay);
        void startSequenceCylon(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color);
        uint32_t dimColor(uint32_t color, uint8_t width);
};
#endif //__LED_CONTROLLER_H__