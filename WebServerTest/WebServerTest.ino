#include <WiFi.h>
#include <esp_now.h>

bool unlock_pending = false;
bool disengage_pending = false;
bool engage_pending = false;

long last_message_time = 0;
long last_unlock_time = 0;
bool engaged = false;
bool unlocked = false;

long last_broadcast_ms = 0;

uint8_t* auth_peers[256];
int num_peers = 0;

void callback(const uint8_t* mac, const uint8_t* d, int len) {
    Serial.println("Data received from: " + String(mac[0], 16) + ":" + String(mac[1], 16) + ":" + String(mac[2], 16) + ":" + String(mac[3], 16) + ":" + String(mac[4], 16) + ":" + String(mac[5], 16));

    String d_str;
    for (int i = 0; i < len; i++) {
        d_str += (char)d[i];
    }

    Serial.println("Data: " + d_str);

    last_message_time = millis();

    if (!is_peer(mac)) {
        Serial.println("Device isn't a peer.");
        if (d_str == "0016777216")
            add_peer(mac);

        return;
    }
    else
        Serial.println("Device is a peer.");

    if (d_str == "unlock") {
        Serial.println("Unlock requested.");

        unlock_pending = true;
    }

    if (d_str == "disengage") {
        Serial.println("Disengage requested.");

        disengage_pending = true;
    }

    if (d_str == "engage") {
        Serial.println("Engage requested.");

        engage_pending = true;
    }
}

void add_peer(const uint8_t* mac) {
    Serial.println("Adding peer.");
    uint8_t* mac_ = (uint8_t*)malloc(6);
    memcpy(mac_, mac, 6);
    auth_peers[num_peers] = mac_;
    num_peers++;

    esp_now_peer_info_t peer_inf = {};
    peer_inf.channel = 1;
    peer_inf.encrypt = false;
    memcpy(peer_inf.peer_addr, mac, 6);

    esp_now_add_peer(&peer_inf);
}

bool is_peer(const uint8_t* mac) {
    for (int i = 0; i < num_peers; i++) {
        if (memcmp(auth_peers[i], mac, 6) == 0)
            return true;
    }

    return false;
}

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_MODE_STA);
    Serial.println("MAC: " + WiFi.macAddress());

    esp_now_init();
    esp_now_register_recv_cb(callback);

    Serial.println("OK.");

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    delay(250);
    digitalWrite(2, LOW);
}

void loop() {
    delay(100);

    if (unlock_pending) {
        if (!engaged) {
            esp_now_send(NULL, (uint8_t*)"disengaged", 10);
        }
        else if (!unlocked) {
            Serial.println("Unlocking.");

            esp_now_send(NULL, (uint8_t*)"unlocking", 9);

            digitalWrite(2, HIGH);
            delay(1000);
            digitalWrite(2, LOW);

            esp_now_send(NULL, (uint8_t*)"unlocked", 8);
            Serial.println("Unlocked.");

            last_unlock_time = millis();
            unlocked = true;
        }
        unlock_pending = false;
    }

    if (unlocked && (millis() - last_unlock_time > 5000)) {
        Serial.println("Locking.");

        esp_now_send(NULL, (uint8_t*)"locking", 7);

        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);

        esp_now_send(NULL, (uint8_t*)"locked", 6);
        Serial.println("Locked.");
        unlocked = false;
    }

    if (disengage_pending) {
        Serial.println("Disengaging.");

        esp_now_send(NULL, (uint8_t*)"disengaging", 11);

        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);

        disengage_pending = false;
        esp_now_send(NULL, (uint8_t*)"disengaged", 10);
        Serial.println("Disengaged.");

        engaged = false;
    }

    if (engage_pending) {
        Serial.println("Engaging.");

        esp_now_send(NULL, (uint8_t*)"engaging", 8);

        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);

        engage_pending = false;
        esp_now_send(NULL, (uint8_t*)"engaged", 7);
        Serial.println("Engaged.");

        engaged = true;
    }

    if (millis() - last_broadcast_ms > 1000) {
        if (!unlocked && engaged)
            esp_now_send(NULL, (uint8_t*)"locked", 6);
        else if (unlocked && engaged)
            esp_now_send(NULL, (uint8_t*)"unlocked", 8);
        else if (!engaged)
            esp_now_send(NULL, (uint8_t*)"disengaged", 10);

        last_broadcast_ms = millis();
    }

    digitalWrite(2, engaged ^ ((millis() - last_message_time < 250) || (unlocked && millis() % 500 < 250)));
}