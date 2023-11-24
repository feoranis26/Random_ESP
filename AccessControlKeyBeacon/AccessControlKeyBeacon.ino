#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define LED_BUILTIN 22

uint8_t peer_mac[] = /*{0xC0, 0x49, 0xEF, 0xF8, 0x49, 0x0C};*/  /*{0x24, 0x0A, 0xC4, 0xEC, 0x9C, 0x3C};*/{ 0xC8, 0xF0, 0x9E, 0x4E, 0xD3, 0x0C };//68:C6:3A:F8:8B:AB //c8:f0:9e:4e:61:14 //c0:49:ef:f8:49:0c

enum KeyState {
    NEARBY,
    DETECTED,
    APPROACHING,
    NOT_DETECTED,
    RESEND = -1,
    WAIT,
    ERROR
};

KeyState CurrentState = WAIT;


void send_callback(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.println("State: " + String(status));
    if (status != ESP_NOW_SEND_SUCCESS)
        CurrentState = ERROR;
}

void callback(const uint8_t* mac, const uint8_t* d, int len) {
    Serial.println("Data received from: " + String(mac[0], 16) + ":" + String(mac[1], 16) + ":" + String(mac[2], 16) + ":" + String(mac[3], 16) + ":" + String(mac[4], 16) + ":" + String(mac[5], 16));
    CurrentState = (KeyState)atoi(String((char*)d, len).c_str());
    Serial.println("State: " + String(CurrentState));
}

void setup() {
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);


    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    //WiFi.begin("", "", 11, 0, false);

    esp_wifi_set_promiscuous(true);
    //esp_wifi_set_max_tx_power(WIFI_POWER_MINUS_1dBm);
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    Serial.printf("WiFi Channel: %d\n", WiFi.channel());

    Serial.println("MAC: " + WiFi.macAddress());
    int ok = esp_now_init();
    Serial.printf("OK: %d\n", ok);

    esp_now_register_send_cb(send_callback);
    esp_now_register_recv_cb(callback);

    Serial.println("OK.");

    pinMode(LED_BUILTIN, OUTPUT);
    //digitalWrite(1, HIGH);
    //delay(250);
    //digitalWrite(1, LOW);

    esp_now_peer_info peer_info = {};
    memcpy(peer_info.peer_addr, peer_mac, 6);
    peer_info.channel = 6;

    if (esp_now_add_peer(&peer_info) != ESP_OK)
        Serial.println("[ESP-NOW] Failed to add peer.");
    else
        Serial.println("[ESP-NOW] Ok");

    //delay(500);

    Serial.println("sending...");

    esp_now_send(peer_mac, (uint8_t*)"16777217", 8);
    esp_now_send(peer_mac, (uint8_t*)"unlock", 6);

    double battery_volts = 0;
    for (int i = 0; i < 20; i++)
        battery_volts += analogRead(33) * 2.0 / 4096.0 * 3.3 + 0.35;

    battery_volts /= 20;

    Serial.printf("Battery voltage: %f\n", battery_volts);

    ledcSetup(0, 3000, 8);
    ledcAttachPin(32, 0);

    if (battery_volts < 3.3) {
        delay(0);
        digitalWrite(LED_BUILTIN, LOW);
        ledcWrite(0, 128);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        ledcWrite(0, 0);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        ledcWrite(0, 128);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        ledcWrite(0, 0);
        delay(500);
        ESP.deepSleep(UINT32_MAX);
    }
}

long last_sent_ms = -5000;

void loop() {
    if (millis() - last_sent_ms > 1000) {
        esp_now_send(peer_mac, (uint8_t*)"unlock", 6);
        last_sent_ms = millis();
    }

    if (CurrentState == ERROR) {
        Serial.println("Send error, sleeping.");
        ESP.deepSleep(10e6);
    }

    if (CurrentState == APPROACHING) {
        Serial.println("Too far away from door");
        if (millis() - last_sent_ms > 100) {
            esp_now_send(peer_mac, (uint8_t*)"unlock", 6);
            last_sent_ms = millis();
        }

        digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
    }
    else if (CurrentState == DETECTED) {
        ESP.deepSleep(10e6);
    }
    else if (CurrentState == NEARBY) {
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (CurrentState == RESEND) {
        esp_now_send(peer_mac, (uint8_t*)"unlock", 6);
        CurrentState = WAIT;
        last_sent_ms = millis() - 1000;
    }

    delay(10);
}
