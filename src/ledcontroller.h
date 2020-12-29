#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "config.h"

extern Adafruit_NeoPixel pixels;

class LedController {
    public:
        LedController();
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