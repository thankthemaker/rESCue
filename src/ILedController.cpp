#include "ILedController.h"
#include "AppConfiguration.h"

unsigned long idleTimer = 0;

void ILedController::loop(const int *new_forward, const int *new_backward, const int *new_idle, const int *new_brake, const int *new_mall_grab) {

    if(!AppConfiguration::getInstance()->config.lightsSwitch) {
        this->changePattern(Pattern::NONE, *(new_forward) == HIGH, false);
    }
    // is motor brake active?
    if (*(new_brake) == HIGH) {
        // flash backlights
        this->changePattern(Pattern::RESCUE_FLASH_LIGHT, *(new_forward) == HIGH, false);
    }

    // is there a change detected
    if (old_forward != *(new_forward) || old_backward != *(new_backward)) {
        if (esp_log_level_get(LOG_TAG_LED) >= ESP_LOG_DEBUG) {
            ESP_LOGD(LOG_TAG_LED, "change detected: forward is %d was %d, backward is %d was %d",
                     *(new_forward), old_forward, *(new_backward), old_backward);
        }

        this->changePattern(Pattern::FADE, (*new_forward) == HIGH, false);

        old_forward = *(new_forward);
        old_backward = *(new_backward);
    }

    //idle state???
    if (old_idle != *(new_idle) || old_mall_grab != *(new_mall_grab)) {
        if (*(new_idle) == HIGH || old_mall_grab != *(new_mall_grab)) {
            if (idleTimer == 0) {
                idleTimer = millis();
            }
            if (AppConfiguration::getInstance()->config.mallGrab && *(new_mall_grab) == HIGH)
            {
                this->changePattern(Pattern::BATTERY_INDICATOR, true, false);
            }
            else
            {
                this->idleSequence();
            } 
        }
        old_idle = *(new_idle);
        old_mall_grab = *(new_mall_grab);
    }

    if (idleTimer != 0 && AppConfiguration::getInstance()->config.idleLightTimeout > 0 &&
      millis() - idleTimer > AppConfiguration::getInstance()->config.idleLightTimeout) {
        ESP_LOGI(LOG_TAG_LED, "Turn off idle light");
        this->changePattern(NONE, true, false);
        idleTimer = 0;
    }
    this->update();
}


