#ifndef CONFIG_H
#define CONFIG_H


//#define LED_WS28xx // enables the light controller for WS28xx (Neopixel)
#define LED_COB // enables the light controller for tri-color COB-stripes

#ifdef LED_WS28xx
 #define PIN_NEOPIXEL 5
#endif

#ifdef LED_COB
 #define DUAL_MOSFET // uncomment if you use two MOSFET to activate color switching
 #define MOSFET_PIN_1 22
 #define MOSFET_PIN_2 23
#endif

#define PIN_FORWARD    18
#define PIN_BACKWARD   19
#define PIN_BRAKE      21
#define BUZPIN         0

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS            16
#define MAX_BRIGHTNESS       100 // allowed values 1-255
#define MAX_BRIGHTNESS_BRAKE 255 // allowed values 1-255

#define BATTERY_PIN              34
#define VOLTAGE_DIVIDER_CONSTANT 22.19 // for calculation see at bottom
// min and max battery voltage as int (voltage * 100)
#define MAX_BATTARY_VOLTAGE      5040  // set your max. battery voltage here
#define MIN_BATTARY_VOLTAGE      4000  // set your min. battery voltage here

#define VESC_BAUD_RATE 115200
#define VESC_RX_PIN 16 //ESP32 UART2-RX
#define VESC_TX_PIN 17 //ESP32 UART2-TX

#define BT_NAME "FunWheel Controller"

#define DEBUG 1

#endif


/*
*** Calibration / Calculation of VOLTAGE_DIVIDER_CONSTANT ***

To measure voltages between 0V and ~100V we need a voltage divider. I use
a voltage divider of a 470k and a 22k resistor.
The 470k is connected between the battery plus and the ESP32 GPIO (e.g. 36)
The 22k is connected between the battery plus and battery minus.
Since the ESP32 Pin also has an resistance, we have to measure and calculate the constant.

Take a power supply and connect it to the voltage divider. Set the voltage to a constant value 
in the upper range of your maximal battery voltage, e.g 50V. Than measure the voltage at the 
outside of the voltage divider. 
You now have to divide the constant voltage with the measured value, e.g

50.4V / 1.822V =  27.69

Do the same at the lower edge of your battery voltage, e.g. 40V

40V / 1.445V = 27.68

Now you have to values for the typical range of your battery. In this example it's 
a value of 27.69 as VOLTAGE_DIVIDER_CONSTANT is pretty ok.

You always have to do the calibration when changing the resistors or the ESP32.
*/