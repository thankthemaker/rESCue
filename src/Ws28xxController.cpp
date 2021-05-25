#include "Ws28xxController.h"
#include <Logger.h>

Ws28xxController::Ws28xxController(uint16_t pixels, uint8_t pin, uint8_t type) 
  : Adafruit_NeoPixel(pixels, pin, type) {}

    
// Update the pattern
void Ws28xxController::update() {
  if(stopPattern)
    return;

  if((millis() - lastUpdate) > interval) { // time to update 
    lastUpdate = millis();
    switch(activePattern) {
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
      index = totalSteps-1;
      onComplete(); // call the comlpetion callback
    }
  }
}

    // Reverse pattern direction
void Ws28xxController::reverse() {
  if (direction == FORWARD) {
    direction = REVERSE;
    index = totalSteps;
  } else {
    direction = FORWARD;
    index = 0;
  }
}
  
void Ws28xxController::onComplete() {
  Logger::notice(LOG_TAG_WS28XX, "Pattern completed ");
  stopPattern = true;
  blockChange = false;
  if(repeat) {
    changePattern(activePattern, true, repeat);
  } else {
    //changePattern(Pattern::NONE, true);
  }
}


void Ws28xxController::changePattern(Pattern pattern, boolean isForward, boolean repeatPattern) {
  if(!repeatPattern && activePattern == pattern && isForward == (direction == Direction::FORWARD)) {
    return;
  }
  if(blockChange) {
    return;
  }
  maxBrightness = config.lightMaxBrightness;
  stopPattern = false;
  repeat = repeatPattern;
  switch(pattern) {
      case RAINBOW_CYCLE:
        rainbowCycle(10, isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      case THEATER_CHASE:
        theaterChase(
          Color(config.lightColorPrimaryRed,
           config.lightColorPrimaryGreen,
           config.lightColorPrimaryBlue), 
          Color(config.lightColorSecondaryRed,
           config.lightColorSecondaryGreen,
           config.lightColorSecondaryBlue), 100);
        break;
      case COLOR_WIPE:
        break;
      case CYLON:
        cylon(Color(
          config.lightColorSecondaryRed,
          config.lightColorSecondaryGreen,
          config.lightColorSecondaryBlue), 55);
        break;
      case FADE:
        fadeLight(map( config.lightFadingDuration , 0, 500, 1, 15), 
           isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      case RESCUE_FLASH_LIGHT:
        flashLight(80, isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      default:
        break; 
  }
  if(Logger::getLogLevel() == Logger::NOTICE) {
    char buf[64];
    snprintf(buf, 64, "changed light pattern to %d", pattern);
    Logger::notice(LOG_TAG_WS28XX, buf);
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
  for(int i=0; i< numPixels(); i++) {
    setPixelColor(i, wheel(((i * 256 / numPixels()) + index) & 255));
  }
}

void Ws28xxController::flashLight(uint8_t timeinterval, Direction dir) {
  activePattern = Pattern::RESCUE_FLASH_LIGHT;
  interval = timeinterval;
  totalSteps = 10;
  index = 0;
  direction = dir;
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "flash %s", direction == FORWARD ? "forward" : "backward");
    Logger::verbose(LOG_TAG_WS28XX, buf);
  }
}

void Ws28xxController::flashLightUpdate() {
  for(int i = 0; i < numPixels(); i++ ) {
    if(i < numPixels()/2)
      if(direction ==FORWARD) {
#ifdef LED_MODE_ODD_EVEN
        if(i%2 == 0) {
#endif
          setPixelColor(i, Color(maxBrightness, maxBrightness, maxBrightness));
#ifdef LED_MODE_ODD_EVEN
        } else {
          setPixelColor(i, Color(0, 0, 0));
        }
#endif
      } else {
        setPixelColor(i, Color(index%2 == 0 ? MAX_BRIGHTNESS_BRAKE : maxBrightness, 0, 0));
      } 
    else
      if(direction ==FORWARD) {
        setPixelColor(i, Color(index%2 == 0 ? MAX_BRIGHTNESS_BRAKE : maxBrightness, 0, 0));
      } else {
#ifdef LED_MODE_ODD_EVEN
          if(i%2 == 0) {
#endif
          setPixelColor(i, Color(maxBrightness, maxBrightness, maxBrightness));
#ifdef LED_MODE_ODD_EVEN
          } else {
            setPixelColor(i, Color(0, 0, 0));
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
  index = dir == Direction::FORWARD ? 0 :  totalSteps-1;
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "fade %s", direction == FORWARD ? "forward" : "backward");
    Logger::verbose(LOG_TAG_WS28XX, buf);
  }
}

void Ws28xxController::fadeLightUpdate() {
  setLight(direction == Direction::FORWARD, index);
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
void Ws28xxController::theaterChaseUpdate(){
  for(int i=0; i< numPixels(); i++) {
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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Ws28xxController::wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return Color(255 - wheelPos * 3, 0, wheelPos * 3);
  } else if(wheelPos < 170) {
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
  for(int i = 0; i < numPixels(); i++ ) {
    setPixelColor(i, Color(0, 0, 0));
  }
  show();
}

void Ws28xxController::setLight(boolean forward, int brightness) {
#ifdef LED_MODE_ODD_EVEN
  int calc_even = forward ? brightness : brightness - 1;
  int calc_odd  = totalSteps - brightness - 1;
  for(int i = 0; i < numPixels()/2; i++ ) {
    setPixelColor(i, Color(0, 0, 0));
    if(i%2 == 0) {
        setPixelColor(i, Color(calc_even , calc_even, calc_even)); 
    } else if(i%2 != 0) {
        setPixelColor(i, Color(calc_odd, 0, 0));
    }
  }
  for(int i = numPixels()/2; i < numPixels(); i++ ) {
    setPixelColor(i, Color(0, 0, 0));
    if(i%2 == 0) {
        setPixelColor(i, Color(calc_even, 0, 0));
    } else if(i%2 != 0) {
        setPixelColor(i, Color(calc_odd, calc_odd, calc_odd));
    }  
  }
#else
  for(int i = 0; i < numPixels(); i++ ) {
    if(i < numPixels()/2){
      if(forward)
        setPixelColor(i, Color(brightness, brightness, brightness));
      else
        setPixelColor(i, Color(maxBrightness-brightness, 0, 0));
    } else {
      if(forward) 
        setPixelColor(i, Color(brightness, 0, 0));
      else
        setPixelColor(i, Color(maxBrightness-brightness, maxBrightness-brightness, maxBrightness-brightness));
    }
  }
#endif
  show();
} 

void Ws28xxController::idleSequence() {
  changePattern(Pattern::THEATER_CHASE, true, true);
}

void Ws28xxController::startSequence() {
  Logger::notice(LOG_TAG_WS28XX, "run startSequence");
  switch (config.startLightIndex){
  case 1:
    theaterChase(
      Color(config.lightColorPrimaryRed,
        config.lightColorPrimaryGreen,
        config.lightColorPrimaryBlue), 
      Color(config.lightColorSecondaryRed,
        config.lightColorSecondaryGreen,
        config.lightColorSecondaryBlue), 100);
    break;
  case 2:
    cylon(
      Color(config.lightColorSecondaryRed,
        config.lightColorSecondaryGreen,
        config.lightColorSecondaryBlue), 55);
    break;
  case 3:
    rainbowCycle(10, Direction::FORWARD);
    break;
  }
  blockChange = true;
}

uint32_t Ws28xxController::dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}