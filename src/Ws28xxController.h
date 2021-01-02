#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "config.h"
#include "ILedController.h"
#include <Adafruit_NeoPixel.h>

#ifndef PIN_NEOPIXEL
 #define PIN_NEOPIXEL   5
#endif

extern Adafruit_NeoPixel pixels;

class Ws28xxController : public ILedController {
    public:
        Ws28xxController();
        void init();
        void fadeIn(boolean isForward);
        void fadeOut(boolean isForward);
        void flash(boolean isForward);
        void stop();
        void startSequence(byte red, byte green, byte blue, int speedDelay);
        void loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward);
    private:
        void forward(byte brightness);
        void backward(byte brightness);
        void setPixel(int pixel, byte red, byte green, byte blue);
        void showStrip();
};
#endif