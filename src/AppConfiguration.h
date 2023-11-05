#ifndef __APP_CONFIGURATION_H__
#define __APP_CONFIGURATION_H__

#include <config.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "visit_struct.hh"
#include "visit_struct_intrusive.hh"

// visitable struct, see C++ Visitor-Pattern and https://github.com/garbageslam/visit_struct
struct Config {
  BEGIN_VISITABLES(Config);
    VISITABLE(String, deviceName);
    VISITABLE(bool, otaUpdateActive);
    VISITABLE(bool, isNotificationEnabled);
    VISITABLE(bool, isBatteryNotificationEnabled);
    VISITABLE(bool, isCurrentNotificationEnabled);
    VISITABLE(bool, isErpmNotificationEnabled);
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
    VISITABLE(int, lightbarTurnOffErpm);
    VISITABLE(int, lightbarMaxBrightness);
    VISITABLE(bool, brakeLightEnabled);
    VISITABLE(int, numberPixelLight);
    VISITABLE(int, numberPixelBatMon);
    VISITABLE(int, vescId);
    VISITABLE(bool, saveConfig);
    VISITABLE(bool, sendConfig);
    VISITABLE(String , ledType);
    VISITABLE(String , lightBarLedType);
    VISITABLE(String , ledFrequency);
    VISITABLE(String , lightBarLedFrequency);
    VISITABLE(bool , isLightBarReversed);
    VISITABLE(bool , isLightBarLedTypeDifferent);
    VISITABLE(int , idleLightTimeout);
    VISITABLE(bool , mallGrab);
    VISITABLE(int , mtuSize);
    VISITABLE(bool , oddevenActive);
    VISITABLE(bool, lightsSwitch);
    VISITABLE(bool, sendConfigFinished);
  END_VISITABLES;
};

class AppConfiguration {
  public: 
    static AppConfiguration* getInstance();
    boolean readPreferences();
    boolean savePreferences();
    boolean readMelodies();
    boolean saveMelodies();
    Config config;

  private:
    AppConfiguration() = default;
    static AppConfiguration *instance; 
    Preferences preferences; 
};
#endif