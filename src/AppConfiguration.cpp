#include "AppConfiguration.h"
#include "config.h"

AppConfiguration* AppConfiguration::instance = nullptr;

AppConfiguration* AppConfiguration::getInstance() {
    if (instance == nullptr){
        instance = new AppConfiguration();
    }

    return instance;
}
void AppConfiguration::readPreferences() {
    String json = "";
    if(!preferences.begin("rESCue", true)) {
        log_e("no config file found");
    } else {
        log_n("found config file");
        json = preferences.getString("config", "");
    }
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, json);
    log_n("readPreferences: %s", json.c_str());
    config.deviceName = doc["deviceName"] | "rESCue";
    config.otaUpdateActive = doc["otaUpdateActive"] | false;
    config.isNotificationEnabled = doc["isNotificationEnabled"] | false;
    config.isBatteryNotificationEnabled = doc["isBatteryNotificationEnabled"] | false;
    config.isCurrentNotificationEnabled = doc["isCurrentNotificationEnabled"] | false;
    config.isErpmNotificationEnabled = doc["isErpmNotificationEnabled"] | false;
    config.minBatteryVoltage = doc["minBatteryVoltage"] | 40.0;
    config.lowBatteryVoltage = doc["lowBatteryVoltage"] | 42.0;
    config.maxBatteryVoltage = doc["maxBatteryVoltage"] | 50.4;
    config.maxAverageCurrent = doc["maxAverageCurrent"] | 40.0;
    config.brakeLightMinAmp = doc["brakeLightMinAmp"] | 4.0;
    config.batteryDrift = doc["batteryDrift"] | 0.0;
    config.startSoundIndex = doc["startSoundIndex"] | 107;
    config.startLightIndex = doc["startLightIndex"] | 2;
    config.batteryWarningSoundIndex = doc["batteryWarningSoundIndex"] | 406;
    config.batteryAlarmSoundIndex = doc["batteryAlarmSoundIndex"] | 402;
    config.startLightDuration = doc["startLightDuration"] | 1000;
    config.lightColorPrimary = doc["lightColorPrimary"] | 0xFFFFFF;
    config.lightColorSecondary = doc["lightColorSecondary"] | 0xFF0000;
    config.idleLightIndex = doc["idleLightIndex"] | 4;
    config.lightFadingDuration = doc["lightFadingDuration"] | 220;
    config.lightMaxBrightness = doc["lightMaxBrightness"] | MAX_BRIGHTNESS;
    config.brakeLightEnabled = doc["brakeLightEnabled"] | true;
    config.authToken = doc["authToken"] | BLYNK_AUTH_TOKEN;
    config.vescId = doc["vescId"] | VESC_CAN_ID;
    config.numberPixelLight = doc["numberPixelLight"] | NUMPIXELS;
    config.numberPixelBatMon = doc["numberPixelBatMon"] | LIGHT_BAR_NUMPIXELS;
    // calculate RGB values for primary and secondary color
    config.lightColorPrimaryRed = (config.lightColorPrimary >> 16) & 0x0ff;
    config.lightColorPrimaryGreen = (config.lightColorPrimary >> 8) & 0x0ff;
    config.lightColorPrimaryBlue =  config.lightColorPrimary & 0x0ff;
    config.lightColorSecondaryRed = (config.lightColorSecondary >> 16) & 0x0ff;
    config.lightColorSecondaryGreen = (config.lightColorSecondary >> 8) & 0x0ff;
    config.lightColorSecondaryBlue = config.lightColorSecondary & 0x0ff;
    config.ledType = doc["ledType"] | "RGB";
    config.ledFrequency = doc["ledFrequency"] | "KHZ800";
    config.idleLightTimeout = doc["idleLightTimeout"] | 60000;
    config.logLevel = doc["logLevel"] | Logger::WARNING;
    config.saveConfig = false;
    config.sendConfig = false;
    preferences.end();
}

void AppConfiguration::savePreferences() {
    preferences.begin("rESCue", false);
    StaticJsonDocument<1024> doc;
    doc["deviceName"] = config.deviceName;
    doc["otaUpdateActive"] = config.otaUpdateActive;
    doc["isNotificationEnabled"] = config.isNotificationEnabled;
    doc["minBatteryVoltage"] = config.minBatteryVoltage;
    doc["lowBatteryVoltage"] = config.lowBatteryVoltage;
    doc["maxBatteryVoltage"] = config.maxBatteryVoltage;
    doc["maxAverageCurrent"] = config.maxAverageCurrent;
    doc["brakeLightMinAmp"] = config.brakeLightMinAmp;
    doc["batteryDrift"] = config.batteryDrift;
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
    doc["authToken"] = config.authToken;
    doc["vescId"] = config.vescId;
    doc["numberPixelLight"] = config.numberPixelLight;
    doc["numberPixelBatMon"] = config.numberPixelBatMon;
    doc["ledType"] = config.ledType;
    doc["ledFrequency"] = config.ledFrequency;
    doc["idleLightTimeout"] = config.idleLightTimeout;
    doc["logLevel"] = config.logLevel;
    String json = "";
    serializeJson(doc, json);
    log_n("savePreferences: %s", json.c_str());
    preferences.putString("config", json);
    preferences.end();
}

