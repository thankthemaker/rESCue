// Playing a digital WAV recording repeatadly using the XTronical DAC Audio library
// prints out to the serial monitor numbers counting up showing that the sound plays 
// independently of the main loop
// See www.xtronical.com for write ups on sound and hardware required

#include "SoundData.h"
#include "XT_DAC_Audio.h"

XT_Wav_Class ForceWithYou(Force);     // create an object of type XT_Wav_Class that is used by 
                                      // the dac audio class (below), passing wav data as parameter.
                                      
XT_DAC_Audio_Class DacAudio(25,0);    // Create the main player class object. 
                                      // Use GPIO 25, one of the 2 DAC pins and timer 0

void setup() {
  Serial.begin(115200);
}


void loop() {
  DacAudio.FillBuffer();              // Fill the sound buffer with data
  if(ForceWithYou.Playing==false)     // if not playing, play again
    DacAudio.Play(&ForceWithYou);  
  Serial.println(DacAudio.BufferUsage());      // Will show results in serial monitor after a few runs around the loop
  delay(1);                           // artificial delay to show some reasonable buffer usage
}
