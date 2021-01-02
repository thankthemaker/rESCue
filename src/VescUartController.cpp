#include "vescuartcontroller.h"

VescUart UART;
HardwareSerial vescSerial2(2);

VescUartController::VescUartController() {}

void VescUartController::init() {
    vescSerial2.begin(VESC_BAUD_RATE, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN, false);   
    UART.setSerialPort(&vescSerial2);
#if DEBUG > 0
    UART.setDebugPort(&Serial);      
#endif
}

void VescUartController::getVescValues() {
    if(UART.getVescValues()) {
      UART.printVescValues();
    } else {
#if DEBUG > 0
        Serial.println("Cannot get values from VESC");
#endif
    }
}