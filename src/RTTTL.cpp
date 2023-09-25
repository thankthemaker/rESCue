#include "RTTTL.h"

// Themes and TV
const char *rtttl_simpsons           = "The Simpsons:d=4,o=5,b=160:c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g,8p,8p,8f#,8f#,8f#,8g,a#.,8c6,8c6,8c6,c6";
const char *rtttl_topgun             = "TopGun:d=4,o=4,b=31:32p,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,16f,d#,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,g#";
const char *rtttl_a_team             = "A-Team:d=8,o=5,b=125:4d#6,a#,2d#6,16p,g#,4a#,4d#.,p,16g,16a#,d#6,a#,f6,2d#6,16p,c#.6,16c6,16a#,g#.,2a#";
const char *rtttl_mission_impossible = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
const char *rtttl_danger_mouse       = "DangerMo:d=4,o=5,b=355:a.,8g,a,8a,p,8a4,8p,d,p,a.,8g,a,8a,p,8a4,8p,d,p,a,a,a#,a#,a#,a#,a#,a#,a#,c6,2a,p,8a4,8p,d,p,a.,8g,a,8a,p,8a4,8p,d,p,a.,8g,a,8a,p,8a4,8p,d,p,a,a,a#,a#,a#,a#,a#,a#,a#,c6,2d6,p,8a4,8p,d,p,a.,8a,2a#.,8a#4,8p,d#,2p,a#,2a#,2f#,2d#,a#.,8a#,2b.,8b4,8p,e,2p,b,2b,2g,2e,b.,8d6,1e.6,e6,8e6,8e";
const char *rtttl_super_man          = "SuperMan:d=4,o=5,b=180:8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,1g6,8p,8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,8a6,2g.6,p,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,8b6,8a6,8b6,2c7,8c6,8c6,8c6,8c6,8c6,2c.6";
const char *rtttl_das_boot           = "DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p";
const char *rtttl_knigh_rider1       = "KnightRider:d=4,o=5,b=63:16e,32f,32e,8b,16e6,32f6,32e6,8b,16e,32f,32e,16b,16e6,d6,8p,16e,32f,32e,8b,16e6,32f6,32e6,8b,16e,32f,32e,16b,16e6,f6";
const char *rtttl_knight_rider2      = "Kn-Rider:d=4,o=6,b=90:16d.5,32d#.5,32d.5,8a.5,16d.,32d#.,32d.,8a.5,16d.5,32d#.5,32d.5,16a.5,16d.,2c,16d.5,32d#.5,32d.5,8a.5,16d.,32d#.,32d.,8a.5,16d.5,32d#.5,32d.5,16a.5,16d.,2d#,a4,32a#.4,32a.4,d5,32d#.5,32d.5,2a5,16c.,16d.";
const char *rtttl_star_wars_imperial = "Imperial:d=4,o=5,b=112:8g,16p,8g,16p,8g,16p,16d#.,32p,32a#.,8g,16p,16d#.,32p,32a#.,g,8p,32p,8d6,16p,8d6,16p,8d6,16p,16d#.6,32p,32a#.,8f#,16p,16d#.,32p,32a#.,g,8p,32p,8g6,16p,16g.,32p,32g.,8g6,16p,16f#.6,32p,32f.6,32e.6,32d#.6,16e6,8p,16g#,32p,8c#6,16p,16c.6,32p,32b.,32a#.,32a.,16a#,8p,16d#,32p,8f#,16p,16d#.,32p,32g.,8a#,16p,16g.,32p,32a#.,d6,8p,32p,8g6,16p,16g.,32p,32g.,8g6,16p,16f#.6,32p,32f.6,32e.6,32d#.6,16e6,8p,16g#,32p,8c#6,16p,16c.6,32p,32b.,32a#.,32a.,16a#,8p,16d#,32p,8f#,16p,16d#.,32p,32g.,8g,16p,16d#.,32p,32a#.,g";
const char *rtttl_star_wars_rebel    = "St Wars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6,p,8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6";
const char *rtttl_star_wars_end      = "SW End:d=4,o=5,b=225:2c,1f,2g.,8g#,8a#,1g#,2c.,c,2f.,g,g#,c,8g#.,8c.,8c6,1a#.,2c,2f.,g,g#.,8f,c.6,8g#,1f6,2f,8g#.,8g.,8f,2c6,8c.6,8g#.,8f,2c,8c.,8c.,8c,2f,8f.,8f.,8f,2f";
const char *rtttl_star_wars_cantina  = "Cantina:d=4,o=5,b=250:8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,a,8a,8g#,8a,g,8f#,8g,8f#,f.,8d.,16p,p.,8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,8a,8p,8g,8p,g.,8f#,8g,8p,8c6,a#,a,g";

// Music
const char *rtttl_scooter_howmuch    = "HowMuchI:d=4,o=6,b=125:8g,8g,16f,16e,f,8d.,16c,8d,8g,8g,8f,8e,8g,8g,16f,16e,f,d,8e,8c,2d,8p,8d,8f,8g,a,a,8a_,8g,2a,8g,8g,16f,16e,f,d,8f,8g,8g,8f,8e,8g,8g,16f,16e,f,d,8e,8c,2d";

// Alarm & Siren
const char *rtttl_siren  = "Sirene:d=4,o=6,b=160:2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c,2g5,2c";
const char *rtttl_siren2 = "PoliceSi:d=4,o=6,b=140:8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c";
const char *rtttl_siren3 = "PoliceSi:d=2,o=5,b=160:g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6,g,c6";
const char *rtttl_siren4 = "PoliceAl:d=4,o=6,b=120:a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5,f5,a5";
const char *rtttl_siren5 = "Politie:d=4,o=6,b=112:c5,a5,f5,a5,c5,a5,f5,a5,c5,a5,f5,a5,c5,a5,f5,a5,c5,a5,f5,a5,c5,a5,f5,a5,c5,a5,f5,a5,c5,a5";

// Classic ringtones
const char *rtttl_death_march = "Death March:d=4,o=5,b=125:c.,c,8c,c.,d#,8d,d,8c,c,8c,2c.";
const char *rtttl_hey_baby    = "HeyBaby:d=4,o=5,b=900:8a4,16a#4,16b4,16c,16c#,16d,16d#,16e,16f,16f#,16g,16g#,16a,16a#,16b,16c6,8c#6,16d6,16d#6,16e6,16f6,p,p,16a4,16a#4,16b4,16c,16c#,16d,16d#,16e,16f,16f#,16g,16g#,16a,16a#,16b,16a#,16a,16g#,16g,16f#,16f,16e,16d#,16d,16c#,16c,16b4,16a#4,16a4";

// Simple tones
const char *rtttl_beep_two_short  = "two-short:d=4,o=5,b=100:16e6,16e6";
const char *rtttl_beep_long       = "long:d=1,o=5,b=100:e6";
const char *rtttl_beep_siren      = "siren:d=8,o=5,b=100:d,e,d,e,d,e,d,e";
const char *rtttl_beep_scale_up   = "scale_up:d=32,o=5,b=100:c,c#,d#,e,f#,g#,a#,b";
const char *rtttl_beep_scale_down = "scale_up:d=32,o=5,b=100:b,a#,g#,f#,e,d#,c#,c";
const char *rtttl_beep_positive   = "positive:d=4,o=5,b=100:16c#,a";
const char *rtttl_beep_negative   = "negative:d=4,o=5,b=100:16a,c#";

RTTTL_MELODIES_MAP  RTTTL_MELODIES_VALUES { 
  { RTTTL_MELODIES::SIMPSONS, rtttl_simpsons }, 
  { RTTTL_MELODIES::TOPGUN, rtttl_topgun },
  { RTTTL_MELODIES::A_TEAM, rtttl_a_team }, 
  { RTTTL_MELODIES::MISSION_IMPOSSIBLE, rtttl_mission_impossible }, 
  { RTTTL_MELODIES::DANGER_MOUSE, rtttl_danger_mouse },
  { RTTTL_MELODIES::SUPER_MAN, rtttl_super_man },
  { RTTTL_MELODIES::DAS_BOOT, rtttl_das_boot },
  { RTTTL_MELODIES::KNIGHT_RIDER, rtttl_knigh_rider1 },
  { RTTTL_MELODIES::KNIGHT_RIDER_ALT, rtttl_knight_rider2 },
  { RTTTL_MELODIES::STAR_WARS_IMPERIAL_MARCH, rtttl_star_wars_imperial },
  { RTTTL_MELODIES::STAR_WARS_REBELION, rtttl_star_wars_rebel },
  { RTTTL_MELODIES::STAR_WARS_END, rtttl_star_wars_end },
  { RTTTL_MELODIES::STAR_WARS_CANTINA, rtttl_star_wars_cantina },
  { RTTTL_MELODIES::SCOOTER_HOW_MUCH, rtttl_scooter_howmuch },
  { RTTTL_MELODIES::ALERT_SIREN, rtttl_siren },
  { RTTTL_MELODIES::ALERT_SIREN2, rtttl_siren2 },
  { RTTTL_MELODIES::ALERT_SIREN3, rtttl_siren3 },
  { RTTTL_MELODIES::ALERT_SIREN4, rtttl_siren4 },
  { RTTTL_MELODIES::ALERT_SIREN5, rtttl_siren5 },
  { RTTTL_MELODIES::CLASSIC_DEATH_MARCH, rtttl_death_march },
  { RTTTL_MELODIES::CLASSIC_HEY_BABY, rtttl_hey_baby },
  { RTTTL_MELODIES::SIMPLE_BEEP_TWO_SHORT, rtttl_beep_two_short },
  { RTTTL_MELODIES::SIMPLE_BEEP_ONE_LONG, rtttl_beep_long },
  { RTTTL_MELODIES::SIMPLE_BEEP_SIREN, rtttl_beep_siren },
  { RTTTL_MELODIES::SIMPLE_BEEP_SCALE_UP, rtttl_beep_scale_up },
  { RTTTL_MELODIES::SIMPLE_BEEP_SCALE_DOWN, rtttl_beep_scale_down },
  { RTTTL_MELODIES::SIMPLE_BEEP_POSITIVE, rtttl_beep_positive },
  { RTTTL_MELODIES::SIMPLE_BEEP_NEGATIVE, rtttl_beep_negative },
};