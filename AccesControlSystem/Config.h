#pragma once

#ifndef Config_h
#define Config_h

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "Lock.h"
#include "IO.h"


class Config {
public:
    static void Load();
    static void Save();
    static DynamicJsonDocument Object;
};
#endif