#include "RTTTL.h"

// Simple tones
const char *rtttl_beep_two_short  = "two-short:d=4,o=5,b=100:16e6,16e6";
const char *rtttl_beep_long       = "long:d=1,o=5,b=100:e6";
const char *rtttl_beep_siren      = "siren:d=8,o=5,b=100:d,e,d,e,d,e,d,e";
const char *rtttl_beep_scale_up   = "scale_up:d=32,o=5,b=100:c,c#,d#,e,f#,g#,a#,b";
const char *rtttl_beep_scale_down = "scale_up:d=32,o=5,b=100:b,a#,g#,f#,e,d#,c#,c";
const char *rtttl_beep_positive   = "positive:d=4,o=5,b=100:16c#,a";
const char *rtttl_beep_negative   = "negative:d=4,o=5,b=100:16a,c#";

RTTTL_MELODIES_MAP  RTTTL_MELODIES_VALUES { 
  { RTTTL_MELODIES::SIMPLE_BEEP_TWO_SHORT, rtttl_beep_two_short },
  { RTTTL_MELODIES::SIMPLE_BEEP_ONE_LONG, rtttl_beep_long },
  { RTTTL_MELODIES::SIMPLE_BEEP_SIREN, rtttl_beep_siren },
  { RTTTL_MELODIES::SIMPLE_BEEP_SCALE_UP, rtttl_beep_scale_up },
  { RTTTL_MELODIES::SIMPLE_BEEP_SCALE_DOWN, rtttl_beep_scale_down },
  { RTTTL_MELODIES::SIMPLE_BEEP_POSITIVE, rtttl_beep_positive },
  { RTTTL_MELODIES::SIMPLE_BEEP_NEGATIVE, rtttl_beep_negative },
};