// Playing a digital WAV recording repeatedly at a rate determined by an analog input
// 
// using the XTronical DAC Audio library
// plays first at normal speed, then fast, then slow and then repeats
// See www.xtronical.com for write ups on sound, the hardware required and how to make
// the wav files and include them in your code

#include "SoundData.h"
#include "XT_DAC_Audio.h"

#define ANALOG_PIN 36                    // Labelled as either 36 or VP on you board

XT_DAC_Audio_Class DacAudio(25,0);      // Create the main player class object. 
                                        // Use GPIO 25, one of the 2 DAC pins and timer 0
                                      
XT_Wav_Class WarOfWorlds(WarOfWorldsWav);     // create an object of type XT_Wav_Class that is used by 
                                              // the dac audio class (above), passing wav data as parameter.
                   
void setup() {
  analogReadResolution(10);
  WarOfWorlds.RepeatForever=true;
  DacAudio.Play(&WarOfWorlds);             // Set to play initially at normal speed, will be altered in main loop according to
                                        // the value of the anaog input 
}

void loop() {
  DacAudio.FillBuffer();                // Fill the sound buffer with data, required once in your main loop
  WarOfWorlds.Speed=analogRead(ANALOG_PIN)/512.0;   // Gives us a speed range between 0 and 7
}
