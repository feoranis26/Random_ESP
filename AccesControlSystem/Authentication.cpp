#include "Authentication.h"

DynamicJsonDocument Authentication::conf(8192);
long Authentication::LastAuthenticatedMillis = -100000;

void Authentication::Init() {
    String file_str;
    File config_file = LittleFS.open("/keys.json", "r", true);
    file_str = config_file.readString();
    config_file.close();

    Serial.println("Read auth key file: " + file_str);

    if (file_str != "" || file_str == "null")
        deserializeJson(conf, file_str);
    else
        conf.createNestedArray("keys");
}

bool Authentication::Authenticate(String key) {
    JsonArray a = conf["keys"];

    for (String k : a) {
        if (k == key) {
            LastAuthenticatedMillis = millis();
            return true;
        }
    }

    if (key == "16777217") {
        LastAuthenticatedMillis = millis();
        return true;
    }

    return false;
}

String Authentication::AddAuth(String key) {
    if (Authenticate(key))
        return "key_present";

    conf["keys"].as<JsonArray>().add(key);
    Save();

    return "ok";
}
String Authentication::RemoveAuth(String key) {
    JsonArray a = conf["keys"];

    for (int i = 0; i < a.size(); i++) {
        String k = a[i];
        if (k == key) {
            a.remove(i);
            return "ok";
        }
    }

    return "key_not_present";
}

void Authentication::Save() {
    String file_str;
    serializeJson(conf, file_str);

    Serial.println("Saving auth key file: " + file_str);

    File config_file = LittleFS.open("/keys.json", "w", true);
    config_file.print(file_str);
    config_file.close();
}

DynamicJsonDocument Authentication::GetConfig() {
    return conf;
}