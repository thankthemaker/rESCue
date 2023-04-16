#include "ILedController.h"
#include "Ws28xxController.h"

LedControllerFactory *LedControllerFactory::instance = 0;

LedControllerFactory::LedControllerFactory() = default;

LedControllerFactory *LedControllerFactory::getInstance() {
    if (instance == 0) {
        instance = new LedControllerFactory();
    }

    return instance;
}

ILedController *LedControllerFactory::createLedController(VescData *vescData) {
    uint8_t ledType = LedControllerFactory::determineLedType();
    return new Ws28xxController(AppConfiguration::getInstance()->config.numberPixelLight, PIN_NEOPIXEL, ledType, vescData);
}

uint8_t LedControllerFactory::determineLedType() {
    uint8_t ledType = 0;
    std::string ledTypeStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledTypeStr == "RGB") {
        ledType = NEO_RGB;
    } else if (ledTypeStr == "RBG") {
        ledType = NEO_RBG;
    } else if (ledTypeStr == "GRB") {
        ledType = NEO_GRB;
    } else if (ledTypeStr == "RGBW") {
        ledType = NEO_RGBW;
    } else if (ledTypeStr == "RGWB") {
        ledType = NEO_RGWB;
    } else if (ledTypeStr == "GRBW" ) {
        ledType = NEO_GRBW;
    } else if (ledTypeStr == "GRWB") {
        ledType = NEO_GRWB;
    } else {
        ledType = NEO_RGB;
    }

    std::string ledFreqStr = std::string(AppConfiguration::getInstance()->config.ledType.c_str());
    if (ledFreqStr == "KHZ800") {
        ledType = ledType + 0x0000;
    } else {
        ledType = ledType + 0x0100;
    }
    return ledType;
}
