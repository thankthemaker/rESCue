#include "Buzzer.h"
#include <Logger.h>

#define LOG_TAG_BUZZER "Buzzer"

MelodyPlayer player(BUZPIN, HIGH);
String notes[] = { "C4", "G3", "G3", "A3", "G3", "SILENCE", "B3", "C4" };
Melody startMelody = MelodyFactory.load("melody 1", 175, notes, 8);

Buzzer::Buzzer() {}

void Buzzer::beep(RTTTL_MELODIES selection){
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

void Buzzer::startSequence() {
  ////player.playAsync(startMelody);
  beep(RTTTL_MELODIES::STAR_WARS_END);
}

void Buzzer::alarm() {
  beep(RTTTL_MELODIES::SIMPLE_BEEP_SIREN);
}
