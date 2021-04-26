#include "ILedController.h"
#include "Ws28xxController.h"
#include "CobController.h"

LedControllerFactory* LedControllerFactory::instance = 0;

LedControllerFactory::LedControllerFactory() {}

LedControllerFactory* LedControllerFactory::getInstance() {
    if (instance == 0){
        instance = new LedControllerFactory();
    }

    return instance;
}

ILedController* LedControllerFactory::createLedController() {
#ifdef LED_WS28xx
  return new Ws28xxController(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif //LED_WS28xx
#ifdef LED_COB
  return new CobController();
#endif //LED_COB
}

