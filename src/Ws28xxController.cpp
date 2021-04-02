#include "ws28xxcontroller.h"
#include <Logger.h>

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

Ws28xxController::Ws28xxController() {}

void Ws28xxController::init() {
  Logger::notice(LOG_TAG_WS28XX, "initializing ...");
  pixels.begin(); // This initializes the NeoPixel library.
}

void Ws28xxController::setPixel(int pixel, byte red, byte green, byte blue) {
   pixels.setPixelColor(pixel, pixels.Color(red, green, blue));
}

void Ws28xxController::showStrip() {
   pixels.show();
}

void Ws28xxController::stop() {
  Logger::verbose("stop");
  for(int i = 0; i < NUMPIXELS; i++ ) {
    this->setPixel(i, 0, 0, 0);
  }
  this->showStrip();
}

void Ws28xxController::setLight(boolean forward, byte brightness) {
  for(int i = 0; i < NUMPIXELS; i++ ) {
    if(i < NUMPIXELS/2){
      if(forward)
        this->setPixel(i, brightness, brightness, brightness);
      else
        this->setPixel(i, brightness, 0, 0);
    } else
      if(forward) 
        this->setPixel(i, brightness, 0, 0);
      else
        this->setPixel(i, brightness, brightness, brightness);
  }
  this->showStrip();
} 

void Ws28xxController::fade(int* isForward) {
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "fade %s", *(isForward) ? "forward" : "backward");
    Logger::verbose(LOG_TAG_WS28XX, buf);
  }

  for(int k = MAX_BRIGHTNESS; k >= 0; k--) {
    this->setLight(!*(isForward), k);
    delay(5);
  }
  for(int k = 0; k < MAX_BRIGHTNESS+1; k++) {
    this->setLight(*(isForward), k);
    delay(5);
  }
}

void Ws28xxController::flash(int* isForward) {
  if(Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[64];
    snprintf(buf, 64, "flash %s", *(isForward) ? "forward" : "backward");
    Logger::verbose(LOG_TAG_WS28XX, buf);
  }
  for(int j=0; j<10; j++) {
    for(int i = 0; i < NUMPIXELS; i++ ) {
      if(i < NUMPIXELS/2)
        *(isForward) ? this->setPixel(i, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS): this->setPixel(i, j%2 == 0 ? MAX_BRIGHTNESS_BRAKE : MAX_BRIGHTNESS, 0, 0);
      else
        *(isForward) ? this->setPixel(i, j%2 == 0 ? MAX_BRIGHTNESS_BRAKE : MAX_BRIGHTNESS, 0, 0) : this->setPixel(i, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
    }    
    this->showStrip();
    delay(80);
  }
}

void Ws28xxController::startSequenceChasing(byte red, byte green, byte blue, int speedDelay) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUMPIXELS; i=i+3) {
        this->setPixel(i+q, red, green, blue);    //turn every third pixel on
      }
      this->showStrip();
     
      delay(speedDelay);
     
      for (int i=0; i < NUMPIXELS; i=i+3) {
        this->setPixel(i+q, 0,0,0);        //turn every third pixel off
      }
    }
  }
}

void Ws28xxController::startSequenceCylon(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) {
  uint32_t old_val[NUMPIXELS]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 1; count<NUMPIXELS; count++) {
      pixels.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        pixels.setPixelColor(x-1, old_val[x-1]); 
      }
      showStrip();
      delay(speed);
    }
    for (int count = NUMPIXELS-1; count>=0; count--) {
      pixels.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x<=NUMPIXELS ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        pixels.setPixelColor(x+1, old_val[x+1]);
      }
      showStrip();
      delay(speed);
    }
  }
}

void Ws28xxController::idleSequence() {
  if(millis() - lastPulse < 70) {
    for (int count = 0; count<NUMPIXELS; count++) {
      pixels.setPixelColor(count, pulse);
      showStrip();
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
#if STARTSEQUENCE == 1
  startSequenceChasing(0, 0, MAX_BRIGHTNESS, 100);
#elif STARTSEQUENCE == 2
  startSequenceCylon(4, 40, 4, 0xFF1000);
#endif
}

uint32_t Ws28xxController::dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}