#ifndef CONFIG_H
#define CONFIG_H

#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL   5
#define PIN_FORWARD    18
#define PIN_BACKWARD   19
#define PIN_BRAKE      21
#define BUZPIN         0

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS            16
#define MAX_BRIGHTNESS       100 // allowed values 1-255
#define MAX_BRIGHTNESS_BRAKE 255 // allowed values 1-255

#define MAX_BATTARY_VOLTAGE 4200 // set your max. battery voltage here
#define MIN_BATTARY_VOLTAGE 3600 // set your max. battery voltage here

#define VESC_BAUD_RATE 115200
#define VESC_RX_PIN 16 //ESP32 UART2-RX
#define VESC_TX_PIN 17 //ESP32 UART2-TX

#define BT_NAME "FunWheel Controller"

//#define DEBUG

#endif