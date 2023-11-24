#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include <WebServer.h>
#include <Update.h>
#include <ElegantOTA.h>
#include <esp_crt_bundle.h>
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTcp.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <ESP32Encoder.h>
#include <MFRC522.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include <SimpleFTPServer.h>
#include <AsyncUDP.h>
#include <CircularBuffer.h>
#include <RemoteLogging.h>

#include "espnow_support.h"
#include "AccessControlGUI.h"
#include "IO.h"
#include "Authentication.h"
#include "Comms.h"


TFT_eSPI screen = TFT_eSPI();

AsyncWebServer ota_server(81);


void setup() {
    Serial.begin(115200);
    LittleFS.begin(true);

    SPI.begin();

    WiFi.config(IPAddress(10, 134, 102, 226), IPAddress(10, 134, 103, 254), IPAddress(255, 255, 0, 0), IPAddress(10, 134, 103, 254), IPAddress(8, 8, 8, 8));
    WiFi.begin("BK_School", "8K-$cH0oL!");
    WiFi.softAP("FRC_LOCK", "16777216", 6);
    //WiFi.mode(WIFI_AP_STA);
    //WiFi.begin("ARMATRON_NETWORK", "16777216");
    //WiFi.mode(WIFI_STA);
    Serial.println("WiFi MAC    : " + WiFi.macAddress());
    Serial.printf("WiFi Channel: %d\n", WiFi.channel());

    Config::Load();

    initGUI();
    Lock::init();
    IO_fx::Init();
    Authentication::Init();
    setupServer();
    setup_esp_now();
    ElegantOTA.begin(&ota_server);

    WiFi.waitForConnectResult();

    ota_server.on("/", [](AsyncWebServerRequest* r) {
        r->send(200, "text/html", "Update server ok, build date/time " + String(__DATE__) + String(__TIMESTAMP__) + ", uptime: " + String(millis()));
        });
    ota_server.begin();

    RemoteLog.begin(IPAddress(10, 134, 101, 5), 11752);

    Serial.printf("WiFi Channel: %d\n", WiFi.channel());
}

// the loop function runs over and over again until power down or reset
void loop() {
    //vTaskDelete(NULL);
    delay(100);
    ElegantOTA.loop();
}
