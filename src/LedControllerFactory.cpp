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
    uint8_t ledType = LedControllerFactory::determineLedType();
    return new Ws28xxController(AppConfiguration::getInstance()->config.numberPixelLight, PIN_NEOPIXEL, ledType);
#endif //LED_WS28xx
#ifdef LED_COB
    return new CobController();
#endif //LED_COB
}

uint8_t LedControllerFactory::determineLedType() {
    uint8_t ledType = 0;
    std::string ledTypeStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledTypeStr == "RGB") {
        ledType = 6;
    } else if (ledTypeStr == "GRB") {
        ledType = 82;
    } else if (ledTypeStr == "RGBW") {
        ledType = 198;
    } else if (ledTypeStr == "GRBW" ) {
        ledType = 210;
    }

    std::string ledFreqStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledFreqStr == "KHZ800") {
        ledType = ledType + 0x0000;
    } else {
        ledType = ledType + 0x0100;
    }
    return ledType;
}
