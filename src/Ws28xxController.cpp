#include "Ws28xxController.h"
#include <Logger.h>

//stuff for using seperate front and back pins. 
Ws28xxController::Ws28xxController(uint16_t pixels, uint8_t frontPin, uint8_t backPin, uint8_t type, VescData *vescData)
        : frontStrip(pixels / 2, frontPin, type), backStrip(pixels / 2, backPin, type), useTwoPins(true), vescData(vescData) {
} 
// Constructor for single-pin configuration
Ws28xxController::Ws28xxController(uint16_t pixels, uint8_t pin, uint8_t type, VescData *vescData)
        : frontStrip(pixels, pin, type), useTwoPins(false), vescData(vescData) {
    backStrip = Adafruit_NeoPixel(); // Explicitly set backStrip to an empty instance
}

// setPixelColor method
void Ws28xxController::setPixelColor(uint16_t n, uint32_t c) {
    if (useTwoPins) {
        if (n < frontStrip.numPixels()) {
            frontStrip.Adafruit_NeoPixel::setPixelColor(n, c); // Correct call to the underlying library's method
        } else {
            backStrip.Adafruit_NeoPixel::setPixelColor(n - frontStrip.numPixels(), c); // Correct call to the underlying library's method
        }
    } else {
        frontStrip.Adafruit_NeoPixel::setPixelColor(n, c); // Correct call to the underlying library's method
    }
}

void Ws28xxController::show() {
    frontStrip.show();
    if (useTwoPins) backStrip.show();
}

void Ws28xxController::setPixelColor(uint16_t n, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    uint32_t color = Color(red, green, blue, white);
    setPixelColor(n, color);
}

uint16_t Ws28xxController::numPixels() const {
    return useTwoPins ? frontStrip.numPixels() + backStrip.numPixels() : frontStrip.numPixels();
}

// Update the pattern
void Ws28xxController::update() {
    if (stopPattern)
        return;

    if ((millis() - lastUpdate) > interval) { // time to update
        lastUpdate = millis();

        switch (activePattern) {
            case RAINBOW_CYCLE:
                rainbowCycleUpdate();
                break;
            case THEATER_CHASE:
                theaterChaseUpdate();
                break;
            case COLOR_WIPE:
                //ColorWipeUpdate();
                break;
            case CYLON:
                cylonUpdate();
                break;
            case FADE:
                fadeLightUpdate();
                break;
            case RESCUE_FLASH_LIGHT:
                flashLightUpdate();
                break;
            case PULSE:
                pulsatingLightUpdate();
                break;
            case SLIDE:
                slidingLightUpdate();
                break;
            case BATTERY_INDICATOR:
                batteryIndicatorUpdate();
                break;
            default:
                break;
        }
        show();
        increment();
    }
}

// Increment the Index and reset at the end
void Ws28xxController::increment() {
    if (direction == FORWARD) {
        index++;
        if (index >= totalSteps) {
            index = 0;
            onComplete(); // call the comlpetion callback
        }
    } else { // Direction == REVERSE
        --index;
        if (index <= 0) {
            index = totalSteps - 1;
            onComplete(); // call the comlpetion callback
        }
    }
}

// Reverse pattern direction
void Ws28xxController::reverse() {
    if (Logger::getLogLevel() == Logger::VERBOSE) {
      char buf[128];
      snprintf(buf, 128, "reversing pattern %d, direction %d", activePattern, direction);
      Logger::warning(LOG_TAG_WS28XX, buf);
    }
    if (direction == FORWARD) {
        direction = REVERSE;
        index = totalSteps;
    } else {
        direction = FORWARD;
        index = 0;
    }
}

void Ws28xxController::onComplete() {
    if (Logger::getLogLevel() == Logger::VERBOSE) {
      char buf[128];
      snprintf(buf, 128, "onComplete pattern %d, startSequence %d, reverseonComplete %d, repeat %d",
             activePattern, isStartSequence, reverseOnComplete, repeat);
      Logger::verbose(LOG_TAG_WS28XX, buf);
    }
    stopPattern = true;
    blockChange = false;
    if(isStartSequence) {
        isStartSequence = false;
        idleSequence();
        return;
    }
    if(reverseOnComplete){
        reverse();
        stopPattern = false;
        return;
    }
    if(repeat) {
        changePattern(activePattern, true, repeat);
        return;
    }
}

void Ws28xxController::changePattern(Pattern pattern, boolean isForward, boolean repeatPattern) {
    if (!repeatPattern && activePattern == pattern && isForward == (direction == Direction::FORWARD)) {
        return;
    }

    if (blockChange) {
        return;
    }

    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[128];
        snprintf(buf, 128, "changePattern new pattern %d, forward %d, repeat %d", pattern, isForward, repeatPattern);
        Logger::verbose(LOG_TAG_WS28XX, buf);
    }

    maxBrightness = config.lightMaxBrightness;
    stopPattern = false;
    repeat = repeatPattern;
    reverseOnComplete = false;
    switch (pattern) {
        case RAINBOW_CYCLE:
            rainbowCycle(10, isForward ? Direction::FORWARD : Direction::REVERSE);
            break;
        case THEATER_CHASE:
            theaterChase(
                    Color((config.lightColorPrimaryRed * maxBrightness) >> 8,
                          (config.lightColorPrimaryGreen * maxBrightness) >> 8,
                          (config.lightColorPrimaryBlue * maxBrightness) >> 8),
                    Color((config.lightColorSecondaryRed * maxBrightness) >> 8,
                          (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                          (config.lightColorSecondaryBlue * maxBrightness) >> 8), (uint8_t)400);
            break;
        case COLOR_WIPE:
            break;
        case CYLON:
            cylon(Color(
                    (config.lightColorSecondaryRed * maxBrightness) >> 8,
                    (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                    (config.lightColorSecondaryBlue * maxBrightness) >> 8), 55);
            break;
        case FADE:
            fadeLight(map(config.lightFadingDuration, 0, 500, 1, 15),
                      isForward ? Direction::FORWARD : Direction::REVERSE);
            break;
        case RESCUE_FLASH_LIGHT:
            flashLight(80, isForward ? Direction::FORWARD : Direction::REVERSE);
            break;
        case PULSE:
            pulsatingLight(40);
            reverseOnComplete = true;
            break;
        case SLIDE:
            slidingLight(Color((config.lightColorPrimaryRed * maxBrightness) >> 8,
                               (config.lightColorPrimaryGreen * maxBrightness) >> 8,
                               (config.lightColorPrimaryBlue * maxBrightness) >> 8),
                         Color((config.lightColorSecondaryRed * maxBrightness) >> 8,
                               (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                               (config.lightColorSecondaryBlue * maxBrightness) >> 8),
                         config.startLightDuration / (numPixels() / 4));
            break;
        case BATTERY_INDICATOR:
            clear();
            show();
            batteryIndicator(1000);
            break;
        case NONE:
            stop();
            break;
        default:
            break;
    }
}

// Initialize for a RainbowCycle
void Ws28xxController::rainbowCycle(uint8_t timeinterval, Direction dir) {
    activePattern = Pattern::RAINBOW_CYCLE;
    interval = timeinterval;
    totalSteps = 255;
    index = 0;
    direction = dir;
}

// Update the Rainbow Cycle Pattern
void Ws28xxController::rainbowCycleUpdate() {
    for (int i = 0; i < numPixels(); i++) {
        setPixelColor(i, wheel(((i * 256 / numPixels()) + index) & 255));
    }
}

void Ws28xxController::flashLight(uint8_t timeinterval, Direction dir) {
    activePattern = Pattern::RESCUE_FLASH_LIGHT;
    interval = timeinterval;
    totalSteps = 10;
    index = 0;
    direction = dir;
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[64];
        snprintf(buf, 64, "flash %s", direction == FORWARD ? "forward" : "backward");
        Logger::verbose(LOG_TAG_WS28XX, buf);
    }
}

void Ws28xxController::flashLightUpdate() {
    for (int i = 0; i < numPixels(); i++) {
        if (i < numPixels() / 2)
            if (direction == FORWARD) {
#ifdef LED_MODE_ODD_EVEN
                if (i % 2 == 0) {
#endif
                    setPixelColor(i, Color(maxBrightness, maxBrightness, maxBrightness, maxBrightness));
#ifdef LED_MODE_ODD_EVEN
                } else {
                    setPixelColor(i, Color(0, 0, 0, 0));
                }
#endif
            } else {
                setPixelColor(i, Color(index % 2 == 0 ? MAX_BRIGHTNESS_BRAKE : maxBrightness, 0, 0, 0));
            }
        else if (direction == FORWARD) {
            setPixelColor(i, Color(index % 2 == 0 ? MAX_BRIGHTNESS_BRAKE : maxBrightness, 0, 0, 0));
        } else {
#ifdef LED_MODE_ODD_EVEN
            if (i % 2 == 0) {
#endif
                setPixelColor(i, Color(maxBrightness, maxBrightness, maxBrightness, maxBrightness));
#ifdef LED_MODE_ODD_EVEN
            } else {
                setPixelColor(i, Color(0, 0, 0, 0));
            }
#endif
        }
    }
}

void Ws28xxController::fadeLight(uint8_t timeinterval, Direction dir) {
    activePattern = Pattern::FADE;
    interval = timeinterval;
    totalSteps = maxBrightness;
    direction = dir;
    index = dir == Direction::FORWARD ? 0 : totalSteps - 1;
    if (Logger::getLogLevel() == Logger::VERBOSE) {
        char buf[64];
        snprintf(buf, 64, "fade %s", direction == FORWARD ? "forward" : "backward");
        Logger::verbose(LOG_TAG_WS28XX, buf);
    }
}

void Ws28xxController::fadeLightUpdate() {
    setLight(direction == Direction::FORWARD, index);
}

void Ws28xxController::pulsatingLight(uint8_t timeinterval) {
    activePattern = Pattern::PULSE;
    interval = timeinterval;
    totalSteps = maxBrightness / 3;
    direction = Direction::FORWARD;
    index = 0;
}

void Ws28xxController::pulsatingLightUpdate() {
    for (int i = 0; i < numPixels(); i++) {
        if(i<numPixels()/2) {
            setPixelColor(i, Color(index, index, index, index));
        } else {
            setPixelColor(i, Color(index, 0, 0, 0));
        }
    }
}

// Initialize for a Theater Chase
void Ws28xxController::theaterChase(uint32_t col1, uint32_t col2, uint8_t timeinterval, Direction dir) {
    activePattern = Pattern::THEATER_CHASE;
    interval = timeinterval;
    totalSteps = numPixels();
    color1 = col1;
    color2 = col2;
    index = 0;
    direction = dir;
}

// Update the Theater Chase Pattern
void Ws28xxController::theaterChaseUpdate() {
    for (int i = 0; i < numPixels(); i++) {
        if ((i + index) % 3 == 0) {
            setPixelColor(i, color1);
        } else {
            setPixelColor(i, color2);
        }
    }
}

// Initialize for a cylon
void Ws28xxController::cylon(uint32_t col1, uint8_t timeinterval) {
    activePattern = Pattern::CYLON;
    interval = timeinterval;
    totalSteps = (numPixels() - 1) * 2;
    color1 = col1;
    index = 0;
    direction = Direction::FORWARD;
}

// Update the cylon Pattern
void Ws28xxController::cylonUpdate() {
    for (int i = 0; i < numPixels(); i++) {
        if (i == index) { // Scan Pixel to the right
            setPixelColor(i, color1);
        } else if (i == totalSteps - index) { // Scan Pixel to the left
            setPixelColor(i, color1);
        } else { // Fading tail
            setPixelColor(i, dimColor(getPixelColor(i), 4));
        }
    }
}

void Ws28xxController::slidingLight(uint32_t col1, uint32_t col2, uint16_t timeinterval) {
    activePattern = Pattern::SLIDE;
    interval = timeinterval;
    totalSteps = numPixels() / 4;
    color1 = col1;
    color2 = col2;
    index = 0;
    direction = Direction::FORWARD;
}

void Ws28xxController::slidingLightUpdate() {
    setPixelColor(index, color1);
    setPixelColor(numPixels()/2 - 1 - index, color1);
    setPixelColor(numPixels()/2 + index, color2);
    setPixelColor(numPixels()-1 - index, color2);
}

void Ws28xxController::batteryIndicator(uint16_t timeinterval) {
    activePattern = Pattern::BATTERY_INDICATOR;
    interval = timeinterval;
    totalSteps = 100;
    index = 0;
    direction = Direction::FORWARD;
}

void Ws28xxController::batteryIndicatorUpdate() {
    float voltage = vescData->inputVoltage;
    int min_voltage = AppConfiguration::getInstance()->config.minBatteryVoltage * 100;
    int max_voltage = AppConfiguration::getInstance()->config.maxBatteryVoltage * 100;
    int voltage_range = max_voltage - min_voltage;
    int used = max_voltage - voltage * 100; // calculate how much the voltage has dropped
    int value = voltage_range - used; // calculate the remaining value to lowest voltage
    float diffPerPixel = voltage_range / (numPixels() / 2); // calculate how much voltage a single pixel shall represent
    float count = value / diffPerPixel; // calculate how many pixels must shine

    int whole = count; // number of "full" green pixels
    int remainder = value*100 / voltage_range; // percentage of usage of current pixel

    Serial.printf("batteryIndicatorUpdate: voltage %f, voltage_range %d, diffPerPixel %f, pixel %d, count %f, used %d, value %d, remainder %d",
                  voltage, voltage_range, diffPerPixel, numPixels(), count, used, value, remainder);
    Serial.println();
    for(int i=0; i<numPixels();i++) {
        if(value<0) {
            setPixelColor(i, MAX_BRIGHTNESS, 0, 0, 0);
            continue;
        }
        if (i < numPixels() / 2) {
            if (i <= whole) {
                int val = calcVal(remainder);
                setPixelColor(i, MAX_BRIGHTNESS - val, val, 0, 0);
            } else {
                setPixelColor(i, 0, 0, 0, 0);
            }
        } else {
            if (i <= whole+numPixels()/2) {
                int val = calcVal(remainder);
                setPixelColor(i, MAX_BRIGHTNESS - val, val, 0, 0);
            } else {
                setPixelColor(i, 0, 0, 0, 0);
            }
        }
    }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Ws28xxController::wheel(byte wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return Color(255 - wheelPos * 3, 0, wheelPos * 3);
    } else if (wheelPos < 170) {
        wheelPos -= 85;
        return Color(0, wheelPos * 3, 255 - wheelPos * 3);
    } else {
        wheelPos -= 170;
        return Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
}

void Ws28xxController::init() {
    Logger::notice(LOG_TAG_WS28XX, "initializing ...");
    begin(); // This initializes the NeoPixel library.
    maxBrightness = config.lightMaxBrightness;
    show();
}

void Ws28xxController::stop() {
    Logger::verbose("stop");
    activePattern = NONE;
    for (int i = 0; i < numPixels(); i++) {
        setPixelColor(i, Color(0, 0, 0, 0));
    }
    show();
}

void Ws28xxController::setLight(boolean forward, int brightness) {
#ifdef LED_MODE_ODD_EVEN
    int calc_even = forward ? brightness : brightness - 1;
    int calc_odd = totalSteps - brightness - 1;
    for (int i = 0; i < numPixels() / 2; i++) {
        setPixelColor(i, Color(0, 0, 0, 0));
        if (i % 2 == 0) {
            setPixelColor(i, Color(calc_even, calc_even, calc_even, calc_even));
        } else if (i % 2 != 0) {
            setPixelColor(i, Color(calc_odd, 0, 0, 0));
        }
    }
    for (int i = numPixels() / 2; i < numPixels(); i++) {
        setPixelColor(i, Color(0, 0, 0, 0));
        if (i % 2 == 0) {
            setPixelColor(i, Color(calc_even, 0, 0, 0));
        } else if (i % 2 != 0) {
            setPixelColor(i, Color(calc_odd, calc_odd, calc_odd, calc_odd));
        }
    }
#else
    for(int i = 0; i < numPixels(); i++ ) {
      if(i < numPixels()/2){
        if(forward)
          setPixelColor(i, Color(brightness, brightness, brightness, brightness));
        else
          setPixelColor(i, Color(maxBrightness-brightness, 0, 0, 0));
      } else {
        if(forward)
          setPixelColor(i, Color(brightness, 0, 0, 0));
        else
          setPixelColor(i, Color(maxBrightness-brightness, maxBrightness-brightness, maxBrightness-brightness, maxBrightness-brightness));
      }
    }
#endif
    show();
}

void Ws28xxController::idleSequence() {
    Pattern pattern;
    switch (config.idleLightIndex) {
        case 1:
          pattern = Pattern::THEATER_CHASE;
          break;
        case 2:
          pattern = Pattern::CYLON;
          break;
        case 3:
          pattern = Pattern::RAINBOW_CYCLE;
          break;
        case 4:
          pattern = Pattern::PULSE;
          break;
        case 5:
          pattern = Pattern::BATTERY_INDICATOR;
          break;
        default:
            pattern = Pattern::PULSE;
    }
    changePattern(pattern, true, true);
}

void Ws28xxController::startSequence() {
    Logger::notice(LOG_TAG_WS28XX, "run startSequence");
    blockChange = true;
    isStartSequence = true;
    int timeinterval = 0;
    switch (config.startLightIndex) {
        case 1:
            timeinterval = config.startLightDuration / numPixels();
            theaterChase(
                    Color((config.lightColorPrimaryRed * maxBrightness) >> 8,
                          (config.lightColorPrimaryGreen * maxBrightness) >> 8,
                          (config.lightColorPrimaryBlue * maxBrightness) >> 8),
                    Color((config.lightColorSecondaryRed * maxBrightness) >> 8,
                          (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                          (config.lightColorSecondaryBlue * maxBrightness) >> 8), timeinterval);
            break;
        case 2:
            timeinterval = config.startLightDuration / numPixels();
            cylon(
                    Color((config.lightColorSecondaryRed * maxBrightness) >> 8,
                          (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                          (config.lightColorSecondaryBlue * maxBrightness) >> 8), timeinterval);
            break;
        case 3:
            timeinterval = config.startLightDuration / numPixels();
            rainbowCycle(10, Direction::FORWARD);
            break;
        case 4:
            timeinterval = config.startLightDuration / (numPixels() / 4);
            slidingLight(Color((config.lightColorPrimaryRed * maxBrightness) >> 8,
                          (config.lightColorPrimaryGreen * maxBrightness) >> 8,
                          (config.lightColorPrimaryBlue * maxBrightness) >> 8),
                    Color((config.lightColorSecondaryRed * maxBrightness) >> 8,
                          (config.lightColorSecondaryGreen * maxBrightness) >> 8,
                          (config.lightColorSecondaryBlue * maxBrightness) >> 8), timeinterval);
            break;
    }
}

uint32_t Ws28xxController::dimColor(uint32_t color, uint8_t width) {
    return (((color & 0xFF0000) / width) & 0xFF0000) + (((color & 0x00FF00) / width) & 0x00FF00) +
           (((color & 0x0000FF) / width) & 0x0000FF);
}

// map the remaining value to a value between 0 and MAX_BRIGHTNESS
int Ws28xxController::calcVal(int value) {
    return map(value, 0, 100, 0, MAX_BRIGHTNESS);
}