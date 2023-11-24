#ifndef espnow_support_h
#define espnow_support_h

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <RemoteLogging.h>

#include "Lock.h"

void esp_now_callback(const uint8_t* mac, const uint8_t* d, int len);
void add_peer(const uint8_t* mac);
bool is_peer(const uint8_t* mac);
void setup_esp_now();
void esp_now_task(void* param);

#endif