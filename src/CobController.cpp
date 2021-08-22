#include <Arduino.h>
#include <Logger.h>
#include "CobController.h"

CobController::CobController() = default;

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

void CobController::changePattern(Pattern pattern, boolean isForward, boolean repeatPattern) {
    //if (Logger::getLogLevel() == Logger::VERBOSE) {
    char buf[128];
    snprintf(buf, 128, "changePattern new pattern %d, forward %d, repeat %d", pattern, isForward, repeatPattern);
    Logger::error(LOG_TAG_COB, buf);
    ///}

    if (activePattern == pattern && isForward == (direction == Direction::FORWARD)) {
        return;
    }
    stopPattern = false;
    repeat = repeatPattern;
    reverseOnComplete = false;
    activePattern = pattern;
    interval = 5;
    totalSteps = MAX_BRIGHTNESS;
    if (isForward) {
        direction = Direction::FORWARD;
    } else {
        direction = Direction::REVERSE;
    }
    switch (pattern) {
        case RESCUE_FLASH_LIGHT:
            totalSteps = 5;
            break;
        default:
            index = direction == Direction::FORWARD ? 0 : totalSteps - 1;
            break;
    }
}

void CobController::update() {
    if (stopPattern)
        return;

    if ((millis() - lastUpdate) > interval) { // time to update
        switch (activePattern) {
            case FADE:
                fade();
                break;
            case RESCUE_FLASH_LIGHT:
                flash();
                break;
            default:
                break;
        }
        increment();
        lastUpdate = millis();
    }
}

// Increment the Index and reset at the end
void CobController::increment() {
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

void CobController::onComplete() {
    Serial.print("onComplete: ");
    Serial.print("reverseOnComplete  ");
    Serial.print(reverseOnComplete);
    Serial.print(", repeat ");
    Serial.println(repeat);
    stopPattern = true;
    if (reverseOnComplete) {
        reverse();
        stopPattern = false;
        return;
    }
    if (repeat) {
        changePattern(activePattern, true, repeat);
        return;
    }
}

// Reverse pattern direction
void CobController::reverse() {
    Serial.println("reverse: ");
    if (direction == FORWARD) {
        direction = REVERSE;
        index = totalSteps;
    } else {
        direction = FORWARD;
        index = 0;
    }
}

void CobController::fade() {
#ifdef DUAL_MOSFET
    writePWM(0, index);
    writePWM(1,totalSteps - index - 1);
    Serial.print("direction ");
    Serial.print(direction);
    Serial.print(", val1: ");
    Serial.print(index);
    Serial.print(", val2: ");
    Serial.println(totalSteps - index - 1);
#else
    writePWM(0, MAX_BRIGHTNESS);
#endif
}

void CobController::flash() {
    if (index % 2 == 0) {
        if (direction == FORWARD) {
            writePWM(0, MAX_BRIGHTNESS_BRAKE); // set the brightness LED
        } else {
            writePWM(1, MAX_BRIGHTNESS_BRAKE); // set the brightness LED
        }
    } else {
        writePWM(0, 0); // turn off the front LED
        writePWM(1, 0); // turn off the back LED
    }
}

void CobController::stop() {
    writePWM(0, 0); // turn off the front LED
    writePWM(1, 0); // turn off the back LED  
}

void CobController::startSequence() {
    Logger::notice(LOG_TAG_COB, "run startSequence");
    //changePattern(RESCUE_FLASH_LIGHT, true, true);
}

void CobController::idleSequence() {
    //changePattern(FADE, true, true);
    //totalSteps = MAX_BRIGHTNESS / 2;
    //reverseOnComplete = true;
}

void CobController::writePWM(int channel, int dutyCycle) {
    ledcWrite(channel, dutyCycle);
}