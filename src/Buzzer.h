#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <Arduino.h>
#include <melody_player.h>
#include <melody_factory.h>
#include "RTTTL.h"
#include "AppConfiguration.h"

#ifndef BUZPIN
 #define BUZPIN 0  // digital pin for buzzer
#endif //BUZPIN

class Buzzer {
    public:
        static Buzzer* getInstance();
        static void startSequence();
        static void startUpdateSequence();
        static void finishUpdateSequence();
        static void warning();
        static void alarm();
        static void playSound(RTTTL_MELODIES selection);
        static void stopSound();
        static boolean isPlayingSound();

    private:
        Buzzer() {}
        static Buzzer *instance; 
};
#endif //__BUZZER_H__

