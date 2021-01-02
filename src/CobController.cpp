#include <Arduino.h>
#include "CobController.h"

CobController::CobController() {}

void CobController::init() {
#if DEBUG > 0
  Serial.println("Initializing CobController");
#endif
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

void CobController::fadeIn(boolean isForward){
    for (int i=0; i<MAX_BRIGHTNESS; i=i+5) {
#ifdef DUAL_MOSFET
        ledcWrite(isForward ? 0 : 1, i); // set the for
#else
        ledcWrite(0, i); // set the brightness LED
#endif
        delay(COB_DELAY);
    }
}

void CobController::fadeOut(boolean isForward){
    for (int i=MAX_BRIGHTNESS; i>0; i=i-5) {
#ifdef DUAL_MOSFET
        ledcWrite(isForward ? 0 : 1, i); // set the for
#else
        ledcWrite(0, i); // set the brightness LED
#endif
        delay(COB_DELAY);
    }
}

void CobController::flash(boolean isForward){
    for(int i=0; i<10; i++) {
        ledcWrite(0, MAX_BRIGHTNESS); // set the brightness LED
        ledcWrite(1, MAX_BRIGHTNESS); // set the brightness LED
        delay(COB_DELAY);
        ledcWrite(0, 0); // turn off the front LED
        ledcWrite(1, 0); // turn off the back LED
    }
}

void CobController::stop(){
    ledcWrite(0, 0); // turn off the front LED
    ledcWrite(1, 0); // turn off the back LED  
}

void CobController::startSequence(byte red, byte green, byte blue, int speedDelay){
  fadeIn(true);
  fadeOut(true);
  flash(true);
  fadeIn(true);
  fadeOut(true);
}

void CobController::loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward){
  ; //nothing to do for COB
}
