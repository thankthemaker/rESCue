#ifndef __SOUNDS_H__
#define __SOUNDS_H__

#include "Arduino.h"

typedef enum {
  FORCE = 0,
  JETSON_DOORBELL,
  JETSON_SPACECAR
} SOUNDS;

extern unsigned char PROGMEM force_wav;
extern unsigned char PROGMEM jetsons_doorbell_wav;
extern unsigned char PROGMEM jetsons_spacecar_pass_wav;
#endif
