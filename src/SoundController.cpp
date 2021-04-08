#include "SoundController.h"

SoundController::SoundController() :  
  sound(&force_wav), DacAudio(25, 0) {
    DacAudio.DacVolume=100;
  }

void SoundController::playSound(SOUNDS newSound) {
  switch (newSound) {
    case SOUNDS::FORCE:
      break;  
    case SOUNDS::JETSON_DOORBELL:
      break;  
    case SOUNDS::JETSON_SPACECAR:
      break;
  }
  DacAudio.Play(&sound);  
}
