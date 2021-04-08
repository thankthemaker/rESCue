// Playing a simple melody using the XTronical DAC Audio library
// Demonstrates use of the music score object
// See www.xtronical.com for write ups on sound and for hardware required

#include "MusicDefinitions.h"
#include "XT_DAC_Audio.h"

// Data for the melody. Note followed by optional change in playing length in 1/4 beats. See documentation for more details
int8_t PROGMEM TwinkleTwinkle[] = {
  NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_4,  
  NOTE_SILENCE,BEAT_5,SCORE_END
};

XT_DAC_Audio_Class DacAudio(25,0);                                            // Create the main player class object. Use GPIO 25 (DAC pin) and timer 0
XT_MusicScore_Class Music(TwinkleTwinkle,TEMPO_ALLEGRO,INSTRUMENT_PIANO);     // Music score object, handles tunes. Pass Music Data,Tempo, Instrument 
                                                                              // You can just pass the music data and the rest will default to... well.... default values!

void setup() {
  DacAudio.Play(&Music);           
}

void loop() {
  DacAudio.FillBuffer();          // This needs only be in your main loop once, suggest here at the top.
  // put whatever code you want here that you would normally have on your loop
}
