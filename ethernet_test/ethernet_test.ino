/*
 Name:		ethernet_test.ino
 Created:	5/12/2023 6:31:00 PM
 Author:	Alp D
*/

// the setup function runs once when you press reset or power the board
#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPI.h>


#define ESP32_Ethernet_onEvent            ESP32_ENC_onEvent
#define ESP32_Ethernet_waitForConnect     ESP32_ENC_waitForConnect
#define ETH_SPI_HOST                      SPI2_HOST
#define SPI_CLOCK_MHZ       8

#define USING_W5500           false 
#define USING_W6100           false 
#define USING_ENC28J60        true 
#define _ASYNC_UDP_ESP32_ETHERNET_LOGLEVEL_ 5

#include <WebServer.h>
#include <WebServer_ESP32_ENC.h>
#include <AsyncUDP_ESP32_Ethernet.h>

AsyncUDP udp;

String padded(String data, int length) {
    char* p = (char*)malloc(length + 1);
    memcpy(p, data.c_str(), data.length());

    for (int i = data.length(); i < length; i++)
        p[i] = '#';

    p[length] = 0;

    String s(p, length);
    free(p);
    return s;
}

void RS485_Send(String data) {
    String toSend = padded(padded(String(data.length()), 4) + data , 63) + ";";
    Serial.println("Sending: " + toSend);

    digitalWrite(25, HIGH);
    delayMicroseconds(250);
    Serial1.print(toSend);
    delay(1);
    digitalWrite(25, LOW);
}

void setup() {
    Serial.begin(115200);
    Serial.println("OK");
    Serial.println("Begin.");
    Serial.println(WEBSERVER_ESP32_ENC_VERSION);

    UDP_LOGWARN(F("Default SPI pinout:"));
    UDP_LOGWARN1(F("SPI_HOST:"), ETH_SPI_HOST);
    UDP_LOGWARN1(F("MOSI:"), MOSI_GPIO);
    UDP_LOGWARN1(F("MISO:"), MISO_GPIO);
    UDP_LOGWARN1(F("SCK:"), SCK_GPIO);
    UDP_LOGWARN1(F("CS:"), CS_GPIO);
    UDP_LOGWARN1(F("INT:"), INT_GPIO);
    UDP_LOGWARN1(F("SPI Clock (MHz):"), SPI_CLOCK_MHZ);
    UDP_LOGWARN(F("========================="));

    ESP32_Ethernet_onEvent();
    ETH.begin(MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST);
    ETH.config(IPAddress(192, 168, 1, 50), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    ESP32_Ethernet_waitForConnect();

    Serial.println("OK");
    Serial.setDebugOutput(true);
    Serial.print("AsyncUDPSendReceive started @ IP address: ");
    Serial.println(ETH.localIP());

    udp.connect(IPAddress(192, 168, 1, 31), 11752);
    udp.write((uint8_t*)"aaaa", 4);

    Serial1.begin(2000000, 134217756U, 26, 27);
    pinMode(25, OUTPUT);

    RS485_Send(ETH.localIP().toString());
}

// the loop function runs over and over again until power down or reset
void loop() {
    delay(25);
    RS485_Send(ETH.localIP().toString() + " M:" + String(millis()));
}
