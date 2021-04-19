#ifndef __APP_CONFIGURATION_H__
#define __APP_CONFIGURATION_H__

#include <Preferences.h>
#include <ArduinoJson.h>
#include <Logger.h>

#define LOG_TAG_APPCONFIGURATION "AppConfiguration"

struct Config {
  boolean otaUpdateActive = false;
  boolean isNotificationEnabled = false;
  double minBatteryVoltage = 41.0;
  double maxBatteryVoltage = 50.0;
  int startSoundIndex = 0;
  int startLightIndex = 0;
  int batteryWarningSoundIndex = 406;
  int batteryAlarmSoundIndex = 402;
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