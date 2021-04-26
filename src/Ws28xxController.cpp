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
        //TheaterChaseUpdate();
        break;
      case COLOR_WIPE:
        //ColorWipeUpdate();
        break;
      case SCANNER:
        //ScannerUpdate();
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
    index = totalSteps-1;
  } else {
    direction = FORWARD;
    index = 0;
  }
}
  
void Ws28xxController::onComplete() {
  stopPattern = true;
  //changePattern(Pattern::NONE, true);
}


void Ws28xxController::changePattern(Pattern pattern, boolean isForward) {
  if(activePattern == pattern && isForward == (direction == Direction::FORWARD)) {
    return;
  }
  stopPattern = false;
  switch(pattern) {
      case RAINBOW_CYCLE:
        rainbowCycle(10, isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      case THEATER_CHASE:
        break;
      case COLOR_WIPE:
        break;
      case SCANNER:
        break;
      case FADE:
        fadeLight(10, isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      case RESCUE_FLASH_LIGHT:
        flashLight(80, isForward ? Direction::FORWARD : Direction::REVERSE);
        break;
      default:
        break; 
  }
  //if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "changed light pattern to %d", pattern);
    Logger::notice(LOG_TAG_WS28XX, buf);
  //}
}

    // Initialize for a RainbowCycle
void Ws28xxController::rainbowCycle(uint8_t timeinterval, Direction dir) {
  activePattern = RAINBOW_CYCLE;
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
  show();
  increment();
}

void Ws28xxController::flashLight(uint8_t timeinterval, Direction dir) {
  activePattern = RESCUE_FLASH_LIGHT;
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
  for(int i = 0; i < NUMPIXELS; i++ ) {
    if(i < NUMPIXELS/2)
      if(direction ==FORWARD) {
#ifdef LED_MODE_ODD_EVEN
        if(i%2 == 0)
#endif
          setPixelColor(i, Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS));
      } else {
        setPixelColor(i, Color(index%2 == 0 ? MAX_BRIGHTNESS_BRAKE : MAX_BRIGHTNESS, 0, 0));
      } 
    else
      if(direction ==FORWARD) {
        setPixelColor(i, Color(index%2 == 0 ? MAX_BRIGHTNESS_BRAKE : MAX_BRIGHTNESS, 0, 0));
      } else {
#ifdef LED_MODE_ODD_EVEN
          if(i%2 == 0)
#endif
          setPixelColor(i, Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS));
      }
  }    
  show();
  increment();
}

void Ws28xxController::fadeLight(uint8_t timeinterval, Direction dir) {
  activePattern = Pattern::FADE;
  interval = timeinterval;
  totalSteps = MAX_BRIGHTNESS;
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
  show();
  increment();
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
  show();
}

void Ws28xxController::stop() {
  Logger::verbose("stop");
  for(int i = 0; i < NUMPIXELS; i++ ) {
    setPixelColor(i, Color(0, 0, 0));
  }
  show();
}

void Ws28xxController::setLight(boolean forward, int brightness) {
#ifdef LED_MODE_ODD_EVEN
  for(int i = 0; i < NUMPIXELS/2; i++ ) {
    setPixelColor(i, Color(0, 0, 0));
    if(i%2 == 0) {
        setPixelColor(i, Color(brightness, brightness, brightness)); 
    } else if(i%2 != 0) {
        setPixelColor(i, Color(MAX_BRIGHTNESS - 1 - brightness, 0, 0));
    }
  }
  for(int i = NUMPIXELS/2; i < NUMPIXELS; i++ ) {
    setPixelColor(i, Color(0, 0, 0));
    if(i%2 == 0) {
        setPixelColor(i, Color(brightness, 0, 0));
    } else if(i%2 != 0) {
        setPixelColor(i, Color(MAX_BRIGHTNESS - 1 - brightness, MAX_BRIGHTNESS - 1 - brightness, MAX_BRIGHTNESS - 1 - brightness));
    }  
  }
#else
  for(int i = 0; i < NUMPIXELS; i++ ) {
    if(i < NUMPIXELS/2){
      if(forward)
        setPixelColor(i, Color(brightness, brightness, brightness));
      else
        setPixelColor(i, Color(brightness, 0, 0));
    } else {
      if(forward) 
        setPixelColor(i, Color(brightness, 0, 0));
      else
        setPixelColor(i, Color(brightness, brightness, brightness));
    }
  }
#endif
  show();
} 

void Ws28xxController::startSequenceChasing(byte red, byte green, byte blue, int speedDelay) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUMPIXELS; i=i+3) {
        setPixelColor(i+q, Color(red, green, blue));    //turn every third pixel on
      }
      show();
     
      delay(speedDelay);
     
      for (int i=0; i < NUMPIXELS; i=i+3) {
        setPixelColor(i+q, Color(0,0,0));        //turn every third pixel off
      }
    }
  }
}

void Ws28xxController::startSequenceCylon(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) {
  uint32_t old_val[NUMPIXELS]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 1; count<NUMPIXELS; count++) {
      setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        setPixelColor(x-1, old_val[x-1]); 
      }
      show();
      delay(speed);
    }
    for (int count = NUMPIXELS-1; count>=0; count--) {
      setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x<=NUMPIXELS ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        setPixelColor(x+1, old_val[x+1]);
      }
      show();
      delay(speed);
    }
  }
}

void Ws28xxController::idleSequence() {
  if(millis() - lastPulse < 70) {
    for (int count = 0; count<NUMPIXELS; count++) {
      setPixelColor(count, pulse);
      show();
    }
    if(pulse == 127) {
      up = false;
    } else if(pulse == 0) {
      up = true;
    }
    if(up) {
      pulse++;
    } else {
      pulse--;
    }
  }
  lastPulse = millis();
}


void Ws28xxController::startSequence() {
  Logger::notice(LOG_TAG_WS28XX, "run startSequence");
  switch (AppConfiguration::getInstance()->config.startLightIndex){
  case 1:
    startSequenceChasing(0, 0, MAX_BRIGHTNESS, 100);
    break;
  case 2:
    startSequenceCylon(4, 40, 4, 0xFF1000);
    break;
  }
}

uint32_t Ws28xxController::dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}