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
  return new Ws28xxController();
#endif
#ifdef LED_COB
  return new CobController();
#endif
}

