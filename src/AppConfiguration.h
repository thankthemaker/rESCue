#ifndef __APP_CONFIGURATION_H__
#define __APP_CONFIGURATION_H__

#include <config.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Logger.h>

#define LOG_TAG_APPCONFIGURATION "AppConfiguration"

struct Config {
  boolean otaUpdateActive = false;
  boolean isNotificationEnabled = false;
  double minBatteryVoltage = 41.0;
  double maxBatteryVoltage = MAX_BATTARY_VOLTAGE;
  int startSoundIndex = 0;
  int startLightIndex = 0;
  int batteryWarningSoundIndex = 406;
  int batteryAlarmSoundIndex = 402;
  int startLightDuration = 1000;
  int idleLightIndex = 0;
  int lightFadingDuration = 50;
  int lightMaxBrightness = MAX_BRIGHTNESS;
  int lightColorPrimary = 0;
  int lightColorPrimaryRed = 0;
  int lightColorPrimaryGreen = 0;
  int lightColorPrimaryBlue = 0;
  int lightColorSecondary = 0;
  int lightColorSecondaryRed = 0;
  int lightColorSecondaryGreen = 0;
  int lightColorSecondaryBlue = 0;
  boolean brakeLightEnabled = true;
  int brakeLightMinAmp = 4;
};

class AppConfiguration {
  public: 
    static AppConfiguration* getInstance();
    void readPreferences();
    void savePreferences();
    Config config;

  private:
    AppConfiguration() {}
    static AppConfiguration *instance; 
    Preferences preferences; 
};
#endif