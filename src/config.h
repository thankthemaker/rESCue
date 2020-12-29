#ifndef CONFIG_H
#define CONFIG_H

#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL   23
#define PIN_FORWARD    34
#define PIN_BACKWARD   35
#define PIN_BRAKE      33
#define BUZPIN         0

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS            16
#define MAX_BRIGHTNESS       100 // allowed values 1-255
#define MAX_BRIGHTNESS_BRAKE 255 // allowed values 1-255

#define MAX_BATTARY_VOLTAGE 4200 // set your max. battery voltage here
#define MIN_BATTARY_VOLTAGE 3600 // set your max. battery voltage here

#define DEBUG

#endif