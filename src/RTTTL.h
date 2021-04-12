#ifndef __RTTTL_H__
#define __RTTTL_H__

#include <map>

// Themes and TV
extern const char *rtttl_simpsons;
extern const char *rtttl_topgun;
extern const char *rtttl_a_team;
extern const char *rtttl_mission_impossible;
extern const char *rtttl_danger_mouse;
extern const char *rtttl_super_man;
extern const char *rtttl_das_boot;
extern const char *rtttl_knigh_rider1;
extern const char *rtttl_knight_rider2;
extern const char *rtttl_star_wars_imperial;
extern const char *rtttl_star_wars_rebel;
extern const char *rtttl_star_wars_end;
extern const char *rtttl_star_wars_cantina;

// Music
extern const char *rtttl_scooter_howmuch;

// Alarm & Siren
extern const char *rtttl_siren;
extern const char *rtttl_siren2;
extern const char *rtttl_siren3;
extern const char *rtttl_siren4;
extern const char *rtttl_siren5;

// Classic ringtones
extern const char *rtttl_death_march;
extern const char *rtttl_hey_baby;

// Simple tones
extern const char *rtttl_beep_two_short;
extern const char *rtttl_beep_long;
extern const char *rtttl_beep_siren;
extern const char *rtttl_beep_scale_up;
extern const char *rtttl_beep_scale_down;
extern const char *rtttl_beep_positive;
extern const char *rtttl_beep_negative;

enum class RTTTL_MELODIES {
  SIMPSONS = 100,
  TOPGUN,
  A_TEAM,
  MISSION_IMPOSSIBLE,
  DANGER_MOUSE,
  SUPER_MAN,
  DAS_BOOT,
  KNIGHT_RIDER,
  KNIGHT_RIDER_ALT,
  STAR_WARS_IMPERIAL_MARCH,
  STAR_WARS_REBELION,
  STAR_WARS_END,
  STAR_WARS_CANTINA,
  SCOOTER_HOW_MUCH,

  ALERT_SIREN = 200,
  ALERT_SIREN2,
  ALERT_SIREN3,
  ALERT_SIREN4,
  ALERT_SIREN5,

  CLASSIC_DEATH_MARCH  = 300,
  CLASSIC_HEY_BABY,

  SIMPLE_BEEP_TWO_SHORT = 400,
  SIMPLE_BEEP_ONE_LONG,
  SIMPLE_BEEP_SIREN,
  SIMPLE_BEEP_SCALE_UP,
  SIMPLE_BEEP_SCALE_DOWN,
  SIMPLE_BEEP_POSITIVE,
  SIMPLE_BEEP_NEGATIVE
};

typedef std::map<RTTTL_MELODIES, const char*> RTTTL_MELODIES_MAP;
extern RTTTL_MELODIES_MAP  RTTTL_MELODIES_VALUES;

#endif