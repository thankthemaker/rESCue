#include "ILedController.h"
#include "Ws28xxController.h"
#include "CobController.h"

LedControllerFactory *LedControllerFactory::instance = 0;

LedControllerFactory::LedControllerFactory() {}

LedControllerFactory *LedControllerFactory::getInstance() {
    if (instance == 0) {
        instance = new LedControllerFactory();
    }

    return instance;
}

ILedController *LedControllerFactory::createLedController() {
#ifdef LED_WS28xx
    uint8_t ledType = 0;
    std::string ledTypeStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledTypeStr.compare("RGB") == 0) {
        ledType = 6;
    } else if (ledTypeStr.compare("GRB") == 0) {
        ledType = 82;
    } else if (ledTypeStr.compare("RGBW") == 0) {
        ledType = 198;
    } else if (ledTypeStr.compare("GRBW") == 0) {
        ledType = 210;
    }

    std::string ledFreqStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledFreqStr.compare("KHZ800") == 0) {
        ledType = ledType + 0x0000;
    } else {
        ledType = ledType + 0x0100;
    }
    return new Ws28xxController(AppConfiguration::getInstance()->config.numberPixelLight, PIN_NEOPIXEL, ledType);
#endif //LED_WS28xx
#ifdef LED_COB
    return new CobController();
#endif //LED_COB
}
