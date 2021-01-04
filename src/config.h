#ifndef CONFIG_H
#define CONFIG_H

/**** Definition for control of front- and backlight ****/

// Choose either WS28xx compatible stripe or COB-LED stripe
//#define LED_WS28xx // enables the light controller for WS28xx (Neopixel)
#define LED_COB    // enables the light controller for tri-color COB-stripes

#ifdef LED_WS28xx
 #define PIN_NEOPIXEL 5   // the PIN to DIN of the WS28xx stripe
 #define NUMPIXELS    16  // the number of LEDs if WS28xx is used
#endif
#ifdef LED_COB
 #define DUAL_MOSFET     // uncomment if you use two MOSFET to activate color switching
 #define MOSFET_PIN_1 22 // PIN for the first MOSFET
 #define MOSFET_PIN_2 23 // PIN for the second MOSFET
#endif

#define MAX_BRIGHTNESS       100 // max brightness of LEDs, allowed values 1-255
#define MAX_BRIGHTNESS_BRAKE 255 // max brightness of LEDs for brake signal, allowed values 1-255

/**** Definition of the input PINs for Cheap Focer 2 ****/

#define PIN_FORWARD    18  // PC13 from Cheap Focer 2, HIGH when "wheel" is turning forward
#define PIN_BACKWARD   19  // PC14 from Cheap Focer 2, HIGH when "wheel" is turning backward
#define PIN_BRAKE      21  // PA15 from Cheap Focer 2, HIGH when electronic brake is active (>40A)
#define BUZPIN         0   // PB12 from Cheap Focer 2, the PIN for the buzzer

/**** Definition of the battery parameter for battery monitor ****/

#define BATTERY_PIN              34    // the PIN for analogRead, connected to the voltage divider
// min and max battery voltage as int (voltage * 100)
#define MAX_BATTARY_VOLTAGE      5040  // set your max. battery voltage here
#define MIN_BATTARY_VOLTAGE      4000  // set your min. battery voltage here

// optional WS28xx battery bar params
#define BATTERY_BAR                    // activates a visual WS28xx battery bar, if connected
#define BATTERY_BAR_PIN          5     // the PIN for the battery bar
#define BATTERY_BAR_NUMPIXELS    3     // the number of LEDS of the battery bar
#define VOLTAGE_DIVIDER_CONSTANT 23.54 // for calculation see at bottom

/**** Definition of the UART connection to the Cheap Focer 2 ****/

#define VESC_BAUD_RATE 115200  // BAUD rate of the CF2
#define VESC_RX_PIN 16         //ESP32 UART2-RX, connected to TX of CF2
#define VESC_TX_PIN 17         //ESP32 UART2-TX, connected to RX of CF2

// The name this controller should advertise for BLE
#define BT_NAME "FunWheel Controller"

// enable DEBUG, the higher the number, the more DEBUG output
#define DEBUG 2

#endif


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

Now you have to values for the typical range of your battery. In this example it's 
a value of 27.69 as VOLTAGE_DIVIDER_CONSTANT is pretty ok.

You always have to do the calibration when changing the resistors or the ESP32.

****/