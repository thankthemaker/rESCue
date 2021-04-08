#ifndef __SOUND_CONTROLLER_H__
#define __SOUND_CONTROLLER_H__

#include <Arduino.h>
#include <pgmspace.h>
#include "Sounds.h"
#include "XT_DAC_Audio.h"

class SoundController {
    public:
      SoundController();
      void playSound(SOUNDS sound);

    private:
      XT_Wav_Class sound;
      XT_DAC_Audio_Class DacAudio;
};
#endif