#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t peer_mac[] = { 0x24, 0x0A, 0xC4, 0xEC, 0x9C, 0x3C }; //{ 0xC8, 0xF0, 0x9E, 0x4E, 0x61, 0x14 }; //68:C6:3A:F8:8B:AB //c8:f0:9e:4e:61:14

String state = "init";

bool changed_state = false;
bool prev_disengaged = false;

void send_callback(uint8_t* mac, uint8_t status) {
    Serial.println("State: " + String(status));
    if (status)
        state = "send error";
    
    else if(state == "init")
        state = "wait for reply";
    else if (state == "locked")
        state = "wait for toggle reply";
}

void callback(uint8_t* mac, uint8_t* d, uint8_t len) {
    Serial.println("Data received from: " + String(mac[0], 16) + ":" + String(mac[1], 16) + ":" + String(mac[2], 16) + ":" + String(mac[3], 16) + ":" + String(mac[4], 16) + ":" + String(mac[5], 16));

    String d_str;
    for (int i = 0; i < len; i++) {
        d_str += (char)d[i];
    }

    Serial.println("Data: " + d_str);

    state = d_str;
}

void setup() {
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);


    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.setOutputPower(0);

    wifi_promiscuous_enable(1);
    wifi_set_channel(1);
    wifi_promiscuous_enable(0);

    WiFi.setOutputPower(0);
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

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    ok = esp_now_add_peer(peer_mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

    Serial.printf("OK: %d\n", ok);

    //delay(500);

    Serial.println("sending...");

    ok = esp_now_send(peer_mac, (uint8_t*)"16777217", 8);
    esp_now_send(peer_mac, (uint8_t*)"unlock", 6);

    Serial.printf("OK: %d\n", ok);
}

long last_sent_ms;

void loop() {
    if (state == "init") {
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (state == "wait for reply") {
        digitalWrite(LED_BUILTIN, millis() % 150 < 75);
    }
    else if (state == "unauthorized") {
        digitalWrite(LED_BUILTIN, millis() % 150 < 75);
    }
    else if (state == "unlocked" || state == "disengaged") {
        digitalWrite(LED_BUILTIN, millis() % 500 < 400);
    }
    else if (state == "unlocking" || state == "locking" || state == "engaging" || state == "disengaging") {
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (state == "locked") {
        digitalWrite(LED_BUILTIN, millis() % 500 < 400);
    }
    else if (state == "send error") {
        digitalWrite(LED_BUILTIN, millis() % 100 < 50);
    }

    if (!changed_state && (millis() > 1000) && state == "locked") {
        esp_now_send(NULL, (uint8_t*)"disengage", 9);
        changed_state = true;
    }

    if (!changed_state && (millis() > 1000) && state == "disengaged") {
        esp_now_send(NULL, (uint8_t*)"engage", 6);
        changed_state = true;
        prev_disengaged = true;
    }

    if (changed_state && ((prev_disengaged && state == "engaged") || (!prev_disengaged && state == "disengaged"))) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);

        ESP.rebootIntoUartDownloadMode();
    }

    if (state == "send error") {
        ESP.deepSleep(30e6);
    }
}
