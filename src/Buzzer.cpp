#include "Buzzer.h"

#define LOG_TAG_BUZZER "Buzzer"

MelodyPlayer player(BUZPIN, HIGH);

Buzzer* Buzzer::instance = 0;

Buzzer* Buzzer::getInstance() {
    if (instance == 0){
        instance = new Buzzer();
    }

    return instance;
}

void Buzzer::playSound(RTTTL_MELODIES selection){
  if(RTTTL_MELODIES::NO_TONE == selection) {
    return;
  }
  if(player.isPlaying()) {
    Logger::notice(LOG_TAG_BUZZER, "Still playing melody, abort!");
    return;
  }
  std::map<RTTTL_MELODIES, const char*>::iterator it = RTTTL_MELODIES_VALUES.find(selection);
  if (it == RTTTL_MELODIES_VALUES.end()) {
      Logger::error(LOG_TAG_BUZZER, "Melody not found");
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
  RTTTL_MELODIES val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.startSoundIndex);
  playSound(val);
}

void Buzzer::warning() {
  RTTTL_MELODIES val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.batteryWarningSoundIndex);
  playSound(val);
}

void Buzzer::alarm() {
  RTTTL_MELODIES val = static_cast<RTTTL_MELODIES>(AppConfiguration::getInstance()->config.batteryAlarmSoundIndex);
  playSound(val);
}
