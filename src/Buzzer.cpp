#include "Buzzer.h"

#define LOG_TAG_BUZZER "Buzzer"

MelodyPlayer player(BUZPIN, HIGH);

Buzzer* Buzzer::instance = nullptr;

Buzzer* Buzzer::getInstance() {
    if (instance == nullptr){
        instance = new Buzzer();
    }

    return instance;
}

void Buzzer::playSound(RTTTL_MELODIES selection){
  if(RTTTL_MELODIES::NO_TONE == selection) {
    return;
  }
  if(player.isPlaying()) {
    ESP_LOGI(LOG_TAG_BUZZER, "Still playing melody, abort!");
    return;
  }
  auto it = RTTTL_MELODIES_VALUES.find(selection);
  if (it == RTTTL_MELODIES_VALUES.end()) {
      ESP_LOGE(LOG_TAG_BUZZER, "Melody not found");
      return;
  }
  Melody sound = MelodyFactory.loadRtttlString(it->second);
  player.playAsync(sound);
}

void Buzzer::stopSound() {
  if(player.isPlaying()) {
    player.stop();
  }
}

boolean Buzzer::isPlayingSound() {
  return player.isPlaying();
}

void Buzzer::startSequence() {
  auto val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.startSoundIndex);
  playSound(val);
}

void Buzzer::startUpdateSequence() {
  playSound(RTTTL_MELODIES::START_UPDATE);
}

void Buzzer::finishUpdateSequence() {
  playSound(RTTTL_MELODIES::END_UPDATE);
}

void Buzzer::warning() {
  auto val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.batteryWarningSoundIndex);
  playSound(val);
}

void Buzzer::alarm() {
  auto val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.batteryAlarmSoundIndex);
  playSound(val);
}
