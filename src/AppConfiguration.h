#ifndef __APP_CONFIGURATION_H__
#define __APP_CONFIGURATION_H__

#include <config.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "Logger.h"
#include "visit_struct.hh"
#include "visit_struct_intrusive.hh"

#define LOG_TAG_APPCONFIGURATION "AppConfiguration"

// visitable struct, see C++ Visitor-Pattern and https://github.com/garbageslam/visit_struct
struct Config {
  BEGIN_VISITABLES(Config);
    VISITABLE(String, deviceName);
    VISITABLE(boolean, otaUpdateActive);
    VISITABLE(boolean, isNotificationEnabled);
    VISITABLE(boolean, isBatteryNotificationEnabled);
    VISITABLE(boolean, isCurrentNotificationEnabled);
    VISITABLE(boolean, isErpmNotificationEnabled);
    VISITABLE(double, minBatteryVoltage);
    VISITABLE(double, lowBatteryVoltage);
    VISITABLE(double, maxBatteryVoltage);
    VISITABLE(double, maxAverageCurrent);
    VISITABLE(double, brakeLightMinAmp);
    VISITABLE(double, batteryDrift);
    VISITABLE(int, startSoundIndex);
    VISITABLE(int, startLightIndex);
    VISITABLE(int, batteryWarningSoundIndex);
    VISITABLE(int, batteryAlarmSoundIndex);
    VISITABLE(int, startLightDuration);
    VISITABLE(int, idleLightIndex);
    VISITABLE(int, lightFadingDuration);
    VISITABLE(int, lightMaxBrightness);
    VISITABLE(int, lightColorPrimary);
    VISITABLE(int, lightColorPrimaryRed);
    VISITABLE(int, lightColorPrimaryGreen);
    VISITABLE(int, lightColorPrimaryBlue);
    VISITABLE(int, lightColorSecondary);
    VISITABLE(int, lightColorSecondaryRed);
    VISITABLE(int, lightColorSecondaryGreen);
    VISITABLE(int, lightColorSecondaryBlue);
    VISITABLE(boolean, brakeLightEnabled);
    VISITABLE(int, numberPixelLight);
    VISITABLE(int, numberPixelBatMon);
    VISITABLE(int, vescId);
    VISITABLE(String, authToken);
    VISITABLE(Logger::Level, logLevel);
    VISITABLE(boolean, sendConfig);
    VISITABLE(boolean, saveConfig);
    VISITABLE(String , ledType);
    VISITABLE(String , ledFrequency);
    VISITABLE(int , idleLightTimeout);
  END_VISITABLES;
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