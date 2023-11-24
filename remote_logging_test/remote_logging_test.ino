#include <CircularBuffer.h>
#include <AsyncUDP.h>
#include <WiFi.h>
#include "RemoteLogging.h"

esp_log_level_t net_log_level = ESP_LOG_DEBUG;
AsyncUDP udp;


void net_log(esp_log_level_t level, const char* tag, const char* format, ...) {
    if (level > net_log_level)
        return;

    va_list args;
    va_start(args, format);

    size_t size_string = snprintf(NULL, 0, format, args) + 4;
    char* string = (char*)malloc(size_string);
    vsnprintf(string, size_string, format, args);

    String level_str[] = {"INVALID", "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"};

    size_t size_log_string = snprintf(NULL, 0, "[%6u] [%s] [%s]: %s\n", millis(), tag, level_str[level].c_str(), string) + 4;
    char* log_string = (char*)malloc(size_log_string);
    snprintf(log_string, size_log_string, "[%6u] [%s] [%s]: %s\n", millis(), tag, level_str[level].c_str(), string);

    if(udp.connected())
        udp.print(log_string);
    //esp_log_write(level, tag, (String(format) + "\n").c_str(), args);
    log_printf(log_string);

    va_end(args);
    free(string);
    free(log_string);
}

void task(void* param) {
    net_log(ESP_LOG_VERBOSE, "main", "task");
    delay(100);
    net_log(ESP_LOG_ERROR, "main", "returning");
    vTaskDelete(NULL);
    delay(0);
}

void setup() {
    WiFi.begin("ARMATRON_NETWORK", "16777216");
    ESP_LOGI("main", "connecting");
    WiFi.waitForConnectResult();
    ESP_LOGI("main", "ok");

    RemoteLog.begin(IPAddress(192, 168, 1, 36), 11752);

    //udp.connect(IPAddress(192, 168, 1, 36), 11752);
    RemoteLog.log(ESP_LOG_DEBUG, "main", "test 12345 int: %d", 5);
}

// the loop function runs over and over again until power down or reset
void loop() {
    vTaskDelete(NULL);
    delay(0);
}
