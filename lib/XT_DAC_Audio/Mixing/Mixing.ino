// Demo's how to mix multiple sounds together, whether those sounds are WAVs
// or Music Score's or a whatever, here we demo a mixture, a wav with a music score.
// The wav repeats forever
// See www.xtronical.com for write ups on sound and the hardare required

#include "MusicDefinitions.h"
#include "SoundData.h"
#include "XT_DAC_Audio.h"

// Data for the melody. Note followed by optional change in playing length in 1/4 beats. See documentation for more details
int8_t PROGMEM TwinkleTwinkle[] = {
  NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_4,  
  SCORE_END
};
                                      
XT_DAC_Audio_Class DacAudio(25,0);                                          // Create the main player class object. Use GPIO 25, one of the 2 DAC pins and timer 0
XT_Wav_Class ForceWithYou(Force);                                           // create WAV object and pass in the WAV data                                      
XT_MusicScore_Class Music(TwinkleTwinkle,TEMPO_ALLEGRO,INSTRUMENT_PIANO);   // The music score object, pass in the Music data

void setup() {
  ForceWithYou.RepeatForever=true;       // Set this sample to play over and over again 
  DacAudio.Play(&Music);                 // Play the sequence, will play just the once and then stop, See MultiPlayRepeat example for a never ending version                                            
  DacAudio.Play(&ForceWithYou,true);     // Play the sequence, will play just the once and then stop
}


void loop() {
  DacAudio.FillBuffer();          // This needs only be in your main loop once, suggest here at the top.
  // you could write anything else here that you would normally have in your main loop
}
