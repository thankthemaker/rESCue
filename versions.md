# Firmware versions and types


## Versions
Versions follow [symantic versioning](https://semver.org/). The update mechanism always tries to update to the latest compatible version.

## Types
rESCue firmware does support different hardware setups, therefore different types of the firmware exist.

### regular
The regular type uses CANBUS and supports addressable LEDs (e.g. Neopixel or other WS28.. based)

### cob
The COB type supports COB-LED-modules by driving one or two external MOSFET by PWM.

### uart
The UART type uses UART instead of CANBUS to "talk" to the VESC.

### cob_uart
The cob_uart type supports COB-LED-modules by driving one or two external MOSFET by PWM and uses UART for communication with the VESC.

