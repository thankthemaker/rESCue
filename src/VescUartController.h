#ifndef VESC_UART_CONTROLLER
#define VESC_UART_CONTROLLER

#include "config.h"
#include "VescUart.h"

class VescUartController {
    public:
      VescUartController();
      void init();
      void getVescValues();
};

#endif