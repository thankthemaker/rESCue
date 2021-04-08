// Playing a digital WAV recording repeatedly at different play-back speeds
// using the XTronical DAC Audio library
// plays first at normal speed, then fast, then slow and then repeats
// See www.xtronical.com for write ups on sound, the hardware required and how to make
// the wav files and include them in your code

#include "SoundData.h"
#include "XT_DAC_Audio.h"

#define NORMAL_SPEED 1                  // These are the playback speeds, change to
#define FAST_SPEED 1.5                  // see the effect on the sound sample. 1 is default speed
#define SLOW_SPEED 0.75                  // >1 faster, <1 slower, 2 would be twice as fast, 0.5 half as fast

XT_DAC_Audio_Class DacAudio(25,0);      // Create the main player class object. 
                                        // Use GPIO 25, one of the 2 DAC pins and timer 0
                                      
XT_Wav_Class WarOfWorlds(WarOfWorldsWav);     // create an object of type XT_Wav_Class that is used by 
                                        // the dac audio class (above), passing wav data as parameter.
                                      
// SpeedArray contains the order in which the code will playback the sample at the designated speeds
float SpeedArray[]={NORMAL_SPEED,FAST_SPEED,SLOW_SPEED};              
uint8_t NoOfSpeeds=3;                   // Number of difference speeds in the Speed array above
uint8_t SpeedIdx=0;                     // In effect when the checks in the main loop are made this will increment to 0

void setup() {
  delay(1);                             // Allow system to settle, otherwise garbage can play for first second
}

void loop() {
  DacAudio.FillBuffer();                // Fill the sound buffer with data, required once in your main loop
  // Has it completed?
  if(WarOfWorlds.Playing==false)
  {
    // Not Playing, check if played last speed, if so reset SpeedIdx back to 0
    if(SpeedIdx==NoOfSpeeds)
      SpeedIdx=0;
    WarOfWorlds.Speed=SpeedArray[SpeedIdx];
    DacAudio.Play(&WarOfWorlds);           // Set to play initially at normal speed    
    SpeedIdx++;                         // Move to next position, ready for when this sample has completed 
  }
}
