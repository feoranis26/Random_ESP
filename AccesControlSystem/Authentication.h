#ifndef Authentication_h
#define Authentication_h

#include <ArduinoJson.h>
#include <LittleFS.h>


class Authentication {
public:
    static bool Authenticate(String pass);
    static String AddAuth(String pass);
    static String RemoveAuth(String pass);
    static DynamicJsonDocument GetConfig();
    static void Save();
    static void Init();

    static long LastAuthenticatedMillis;
private:
    static DynamicJsonDocument conf;
};

#endif