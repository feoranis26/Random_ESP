#ifndef Comms_h
#define Comms_h

#include "Authentication.h"
#include "Lock.h"
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include <LittleFS.h>
#include <SimpleFTPServer.h>

extern AsyncWebServer server;
extern FtpServer ftp_server;

void setupServer();
void wifi_alert_task(void* param);
void ftp_task(void* param);
void serial_task(void* param);

#endif