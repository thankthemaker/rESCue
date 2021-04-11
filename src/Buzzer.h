#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "Arduino.h"
#include <melody_player.h>
#include <melody_factory.h>
#include "RTTTL.h"

#ifndef BUZPIN
 #define BUZPIN 0  // digital pin for buzzer
#endif //BUZPIN

class Buzzer {
    public:
        Buzzer();
        void startSequence();
        void alarm();
        void beep(RTTTL_MELODIES selection);
};
#endif //__BUZZER_H__

