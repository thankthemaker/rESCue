#include "AppConfiguration.h"

AppConfiguration* AppConfiguration::instance = 0;

AppConfiguration* AppConfiguration::getInstance() {
    if (instance == 0){
        instance = new AppConfiguration();
    }

    return instance;
}
void AppConfiguration::readPreferences() {
    preferences.begin("rESCue", true);
    String json = preferences.getString("config", "");
    StaticJsonDocument<512> doc;
    deserializeJson(doc, json);
    Logger::verbose(LOG_TAG_APPCONFIGURATION, "readPreferences: ");
    Serial.println("readPreferences: " + json);
    config.otaUpdateActive = doc["otaUpdateActive"] | false;
    config.isNotificationEnabled = doc["isNotificationEnabled"] | false;
    config.minBatteryVoltage = doc["minBatteryVoltage"] | 40.0;
    config.maxBatteryVoltage = doc["maxBatteryVoltage"] | 50.4;
    config.startSoundIndex = doc["startSoundIndex"] | 107;
    config.startLightIndex = doc["startLightIndex"] | 1;
    config.batteryWarningSoundIndex = doc["batteryWarningSoundIndex"] | 406;
    config.batteryAlarmSoundIndex = doc["batteryAlarmSoundIndex"] | 402;
    config.startLightDuration = doc["startLightDuration"] | 1000;
    config.lightColorPrimary = doc["lightColorPrimary"] | 0xFFFFFF;
    config.lightColorSecondary = doc["lightColorSecondary"] | 0xFF0000;
    config.idleLightIndex = doc["idleLightIndex"] | 0;
    config.lightFadingDuration = doc["lightFadingDuration"] | 0;
    config.lightMaxBrightness = doc["lightMaxBrightness"] | 0;
    config.brakeLightEnabled = doc["brakeLightEnabled"] | true;
    config.brakeLightMinAmp = doc["brakeLightMinAmp"] | 4;
    // calculate RGB values for primary and secondary color
    config.lightColorPrimaryRed = (config.lightColorPrimary >> 16) & 0x0ff;
    config.lightColorPrimaryGreen = (config.lightColorPrimary >> 8) & 0x0ff;
    config.lightColorPrimaryBlue =  config.lightColorPrimary & 0x0ff;
    config.lightColorSecondaryRed = (config.lightColorSecondary >> 16) & 0x0ff;
    config.lightColorSecondaryGreen = (config.lightColorSecondary >> 8) & 0x0ff;
    config.lightColorSecondaryBlue = config.lightColorSecondary & 0x0ff;
    preferences.end();
}

void AppConfiguration::savePreferences() {
    preferences.begin("rESCue", false);
    StaticJsonDocument<512> doc;
    doc["otaUpdateActive"] = config.otaUpdateActive;
    doc["isNotificationEnabled"] = config.isNotificationEnabled;
    doc["minBatteryVoltage"] = config.minBatteryVoltage;
    doc["maxBatteryVoltage"] = config.maxBatteryVoltage;
    doc["startSoundIndex"] = config.startSoundIndex;
    doc["startLightIndex"] = config.startLightIndex;
    doc["batteryWarningSoundIndex"] = config.batteryWarningSoundIndex;
    doc["batteryAlarmSoundIndex"] = config.batteryAlarmSoundIndex;
    doc["startLightDuration"] = config.startLightDuration;
    doc["lightColorPrimary"] = config.lightColorPrimary;
    doc["lightColorSecondary"] = config.lightColorSecondary;
    doc["idleLightIndex"] = config.idleLightIndex;
    doc["lightFadingDuration"] = config.lightFadingDuration;
    doc["lightMaxBrightness"] = config.lightMaxBrightness;
    doc["brakeLightEnabled"] = config.brakeLightEnabled;
    doc["brakeLightMinAmp"] = config.brakeLightMinAmp;
    String json = "";
    serializeJson(doc, json);
    preferences.putString("config", json);
    preferences.end();
    Logger::verbose(LOG_TAG_APPCONFIGURATION, "savePreferences: ");
        Serial.println("savePreferences: " + json);

}

