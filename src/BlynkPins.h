#define BLINK_COLOR_GREEN "#23C48E"
#define BLINK_COLOR_RED "#D3435C"

#define VPIN_APP_LIGHT_INDEX           1    // INPUT: The index of the Blynk drop-down-list for start-light pattern
#define VPIN_APP_SOUND_INDEX           2    // INPUT: The index of the Blynk drop-down-list for start-sound (mapped to enum)
#define VPIN_APP_VOLTAGE               3    // OUTPUT: The voltage from VESC
#define VPIN_APP_MAX_BAT_VOLTAGE       4    // INPUT: The maximum voltage, used for alarm-tones and notifications
#define VPIN_APP_MIN_BAT_VOLTAGE       5    // INPUT: The minimum voltage, used for alarm-tones and notifications
#define VPIN_APP_NOTIFICATION          6    // INPUT: Switch local Push-Notifications on/off (ATM only Android)
#define VPIN_APP_BATTERY_WARN_INDEX    7    // INPUT: The index of the Blynk drop-down-list for warn-tone (mapped to enum)
#define VPIN_APP_BATTERY_ALARM_INDEX   8    // INPUT: The index of the Blynk drop-down-list for alarm-tone (mapped to enum)
#define VPIN_APP_STARTLIGHT_DURATION   9    // INPUT: The duration in milliseconds for the start-light-sequence
#define VPIN_APP_LIGHT_COLOR_1         10   // INPUT: The primary color for LED lights (if applicable for pattern)
#define VPIN_APP_LIGHT_COLOR_2         11   // INPUT: The secondary color for LED lights (if applicable for pattern)
#define VPIN_APP_IDLE_LIGHT_INDEX      12   // INPUT: The index of the Blynk drop-down-list for idle-light pattern
#define VPIN_APP_LIGHT_FADING_DURATION 13   // INPUT: The duration of fading effect when changing drive direction
#define VPIN_APP_LIGHT_MAX_BRIGHTNESS  14   // INPUT: The maximum brightness of the LEDs (except brake light, which are always max.)
#define VPIN_APP_ACTIVATE_BRAKE_LIGHT  15   // INPUT: Enable brake lights
 #define VPIN_APP_BRAKE_LIGHT_MIN_AMP  16   // INPUT: Minumum (negative) ampere to enable brake lights
#define VPIN_APP_ACTIVATE_OTA          19   // INPUT: Activate OTA, ESP will reboot in OTA mode. Reset automatically after update


// VESC params
#define VPIN_VESC_DUTY_CYCLE         21  // OUTPUT: The actualnduty-cycle reading from VESC (if CANBUS enabled)
#define VPIN_VESC_ERPM               22  // OUTPUT: The actualnerpm reading from VESC (if CANBUS enabled)
#define VPIN_VESC_CURRENT            23  // OUTPUT: The actual current reading from VESC (if CANBUS enabled)
#define VPIN_VESC_AMP_HOURS          24  // OUTPUT: The used amp-hours from VESC (if CANBUS enabled)
#define VPIN_VESC_AMP_HOURS_CHARGED  25  // OUTPUT: The charged amp-hours from VESC (if CANBUS enabled)
#define VPIN_VESC_WATT_HOURS         26  // OUTPUT: The used watt-hours from VESC (if CANBUS enabled)
#define VPIN_VESC_WATT_HOURS_CHARGED 27  // OUTPUT: The charged watt-hours from VESC (if CANBUS enabled)
#define VPIN_VESC_MOSFET_TEMP        28  // OUTPUT: The actual mosfet-temp reading from VESC (if CANBUS enabled)
#define VPIN_VESC_MOTOR_TEMP         29  // OUTPUT: The actual motor-temp reading from VESC (if CANBUS enabled)
#define VPIN_VESC_INPUT_VOLTAGE      30  // OUTPUT: The actual voltage from VESC (from CANBUS or voltage divider)
#define VPIN_VESC_TACHOMETER         31  // OUTPUT: The actual tochometer reading from VESC (if CANBUS enabled)