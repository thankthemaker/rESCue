#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "Arduino.h"

#ifndef BUZPIN
 #define BUZPIN 0  // digital pin for buzzer
#endif //BUZPIN

class Buzzer {
    public:
        Buzzer();
        void startSequence();
        void alarm();
        void beep(byte number);
};
#endif //__BUZZER_H__

