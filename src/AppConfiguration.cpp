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
    config.startSoundIndex = doc["startSoundIndex"] | 111;
    config.startLightIndex = doc["startLightIndex"] | 1;
    config.batteryWarningSoundIndex = doc["batteryWarningSoundIndex"] | 406;
    config.batteryAlarmSoundIndex = doc["batteryAlarmSoundIndex"] | 402;
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
    String json = "";
    serializeJson(doc, json);
    preferences.putString("config", json);
    preferences.end();
    Logger::verbose(LOG_TAG_APPCONFIGURATION, "savePreferences: ");
        Serial.println("savePreferences: " + json);

}

