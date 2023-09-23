#ifndef __CONFIG_H__
#define __CONFIG_H__

// ###################################################################### //
// ## For board specific GPIO-PIN definition please see platformio.ini ## //
// ###################################################################### //

#define SOFTWARE_VERSION_MAJOR 2
#define SOFTWARE_VERSION_MINOR 4
#define SOFTWARE_VERSION_PATCH 0
#define HARDWARE_VERSION_MAJOR 3
#define HARDWARE_VERSION_MINOR 1

/**** Definition for control of front- and backlight ****/

#define NUMPIXELS    16  // the number of LEDs if WS28xx is used
#define MAX_BRIGHTNESS       100 // max brightness of LEDs, allowed values 1-255
#define MAX_BRIGHTNESS_BRAKE 255 // max brightness of LEDs for brake signal, allowed values 1-255

// optional WS28xx lightbar & battery-monitor params
#define LIGHT_BAR_NUMPIXELS    5     // the number of LEDS of the battery bar

#define LIGHT_BAR_ADC_ENABLED        // Enables the lightbar support for ADC-Footpad

#define VOLTAGE_DIVIDER_CONSTANT 24.527 // for calculation see at bottom

/**** Definition of the UART connection to the Cheap Focer 2 ****/

#define VESC_BAUD_RATE 115200  // BAUD rate of the CF2

#define VESC_CAN_ID 25 //VESC-ID as configured in VESC as decimal

#endif //__CONFIG_H__

/**** Calibration / Calculation of VOLTAGE_DIVIDER_CONSTANT ****

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

Now you have two values for the typical range of your battery. In this example it's 
a value of 27.69 as VOLTAGE_DIVIDER_CONSTANT is pretty ok.

You always have to do the calibration when changing the resistors or the ESP32.

****/