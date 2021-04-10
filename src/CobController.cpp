#include <Arduino.h>
#include <Logger.h>
#include "CobController.h"

CobController::CobController() {}

void CobController::init() {
  Logger::notice(LOG_TAG_COB, "initializing ...");
  ledcAttachPin(MOSFET_PIN_1, 0); // assign a led pins to a channel
#ifdef DUAL_MOSFET
  ledcAttachPin(MOSFET_PIN_2, 1); // assign a led pins to a channel
#endif
  // Initialize channels
  // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  ledcSetup(0, 4000, 8); // 12 kHz PWM, 8-bit resolution
#ifdef DUAL_MOSFET
  ledcSetup(1, 4000, 8); // 12 kHz PWM, 8-bit resolution
#endif
}

void CobController::fade(boolean isForward){
#ifdef DUAL_MOSFET
  for (int i=MAX_BRIGHTNESS; i>0; i=i-5) {
    writePWM(isForward ? 0 : 1, i);
    writePWM(isForward ? 1 : 0, MAX_BRIGHTNESS - i);
    delay(COB_DELAY);
  }
#else
  writePWM(0, MAX_BRIGHTNESS);
#endif
}

void CobController::flash(boolean isForward){
    for(int i=0; i<10; i++) {
        if(isForward) {
          writePWM(0, MAX_BRIGHTNESS_BRAKE); // set the brightness LED
        } else {
          writePWM(1, MAX_BRIGHTNESS_BRAKE); // set the brightness LED
        }
        delay(COB_DELAY);
        writePWM(0, 0); // turn off the front LED
        writePWM(1, 0); // turn off the back LED
    }
}

void CobController::stop(){
    writePWM(0, 0); // turn off the front LED
    writePWM(1, 0); // turn off the back LED  
}

void CobController::startSequence(){
  flash(true);
  fade(true);
}

void CobController::idleSequence(){
}

void CobController::writePWM(int channel, int dutyCycle){
  ledcWrite(channel, dutyCycle);
}