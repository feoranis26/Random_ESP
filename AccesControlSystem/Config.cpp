#include "Config.h"

DynamicJsonDocument Config::Object(4096);

extern int rssi_limit;

void Config::Load()
{
    Serial.println("[Config] Mounting LittleFS.");
    LittleFS.begin(true);

    Serial.println("[Config] Reading file.");

    File config_file = LittleFS.open("/config.json", "r");
    String file_str = config_file.readString();
    config_file.close();

    Serial.println("[Config] Read: \"" + file_str + "\"");

    deserializeJson(Object, file_str);

    rssi_limit = Object["rssi_limit"];

    Lock::ReadData();
    IO_fx::ReadData();
}

void Config::Save()
{
    Serial.println("[Config] Saving file.");

    Lock::UpdateData();
    IO_fx::UpdateData();

    Object["rssi_limit"] = rssi_limit;

    String file_str;
    serializeJson(Object, file_str);

    File config_file = LittleFS.open("/config.json", "w");
    config_file.print(file_str);
    config_file.close();

    Serial.println("[Config] Saved: \"" + file_str + "\"");
}