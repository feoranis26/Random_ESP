#include "espnow_support.h"

uint8_t* auth_peers[256];
int num_peers = 0;

uint8_t rssi_chk_mac[6];
int rssi = 0;
long rssi_validate_ms = 0;
long last_msg_ms = -60000;
long key_near_msg_ms = -60000;

int rssi_limit = -55;
int pkt_count = 0;
int req_pkt_count = 5;

bool opened = false;

enum KeyState {
    NEARBY,
    DETECTED,
    APPROACHING,
    NOT_DETECTED
};

KeyState CurrentState = NOT_DETECTED;

typedef struct {
    unsigned frame_ctrl : 16;
    unsigned duration_id : 16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void IRAM_ATTR promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
    // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
    if (type != WIFI_PKT_MGMT)
        return;

    static const uint8_t ACTION_SUBTYPE = 0xd0;
    static const uint8_t ESPRESSIF_OUI[] = { 0x24, 0x0A, 0xC4 };

    const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buf;
    const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

    // Only continue processing if this is an action frame containing the Espressif OUI.
    if ((ACTION_SUBTYPE == (hdr->frame_ctrl & 0xFF))) {
        if (!is_peer(hdr->addr2))
            return;

        rssi = ppkt->rx_ctrl.rssi;
        memcpy(rssi_chk_mac, hdr->addr2, 6);
        rssi_validate_ms = millis();

        if (rssi == 0)
            rssi = -1000;

        Serial.printf("[ESP-NOW] RSSI       : %d\n", rssi);
        Serial.printf("[ESP-NOW] RSSI Limit : %d\n", rssi_limit);
    }
}

bool is_any_key_near() {
    return (rssi >= rssi_limit) && millis() - rssi_validate_ms < 30000;
}

bool is_key_near(const uint8_t* mac) {
    return is_any_key_near() && !memcmp(mac, rssi_chk_mac, 6);
}

bool is_any_key_detected() {
    return millis() - last_msg_ms < 30000;
}

void esp_now_callback(const uint8_t* mac, const uint8_t* d, int len) {
    //Serial.println("[ESP-NOW] Data received from: " + String(mac[0], 16) + ":" + String(mac[1], 16) + ":" + String(mac[2], 16) + ":" + String(mac[3], 16) + ":" + String(mac[4], 16) + ":" + String(mac[5], 16));

    String d_str;
    for (int i = 0; i < len; i++) {
        d_str += (char)d[i];
    }

    //Serial.println("[ESP-NOW] Data: " + d_str);

    if (!is_peer(mac)) {
        if (!Authentication::Authenticate(d_str)) {
            Serial.println("[ESP-NOW] Unauthorized device!");
            return;
        }

        Serial.println("[ESP-NOW] Device isn't a peer.");
        add_peer(mac);

        return;
    }

    if (millis() - last_msg_ms > 32000)
        IO_fx::OnKeypadWake();

    last_msg_ms = millis();
    //else
        //Serial.println("[ESP-NOW] Device is a peer.");

    if (!is_key_near(mac) && CurrentState != DETECTED) {
        Serial.println("[ESP-NOW] Device is too far away.");

        pkt_count = 0;

        return;
    }

    if (!is_key_near(mac))
        return;

    pkt_count++;

    if(pkt_count < req_pkt_count) {
        esp_now_send(NULL, (uint8_t*)String(-1).c_str(), 1);
        Serial.println("[ESP-NOW] Checking...");

        return;
    }

    key_near_msg_ms = millis();

    /*if (d_str == "unlock") {
        if (Lock::isLocked && !opened_by_key && open_by_key) {
            Lock::UnlockDoor(15000);
            opened_by_key = true;
            keep_open = true;
            door_opened = false;
            open_by_key = false;
            esp_now_send(mac, (uint8_t*)"unlocked_now", 8);
            Serial.println("[ESP-NOW] Unlock requested.");
        }

        if(!Lock::isLocked && keep_open)
            Lock::doorUnlockMillis = millis();
    }

    key_opened_ms = millis();

    if (d_str == "disengage") {
        Serial.println("[ESP-NOW] Disengage requested.");

        Lock::DisengageDoor();
    }

    if (d_str == "engage") {
        Serial.println("[ESP-NOW] Engage requested.");

        Lock::EngageDoor();
    }*/
}

void add_peer(const uint8_t* mac) {
    Serial.println("[ESP-NOW] Adding peer.");
    uint8_t* mac_ = (uint8_t*)malloc(6);
    memcpy(mac_, mac, 6);
    auth_peers[num_peers] = mac_;
    num_peers++;

    esp_now_peer_info_t peer_inf = {};
    peer_inf.channel = 6;
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

void setup_esp_now() {
    ESP_LOGI("esp-now", "Initializing.");
    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_recv_cb(esp_now_callback);

    xTaskCreate(esp_now_task, "esp_now_bcast", 4096, NULL, 0, NULL);
    esp_wifi_set_promiscuous(true);
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb));
}

void esp_now_task(void* param) {
    while (true) {
        delay(500);

        /*if (millis() - key_opened_ms < 10000 && millis() - last_msg_ms > 2000) {
            key_opened_ms = 0;
            Lock::doorUnlockMillis = millis() - 60000;
            //Lock::LockDoor();
        }*/

        if (!is_any_key_detected()) {
            RemoteLog.log(ESP_LOG_VERBOSE, "esp_now", "No beacons detected.");
            pkt_count = 0;
        }

        if (millis() - key_near_msg_ms < 17000) {
            RemoteLog.log(ESP_LOG_VERBOSE, "esp_now", "Beacon is nearby.");
            CurrentState = NEARBY;
        }
        else if (millis() - last_msg_ms < 32000 && CurrentState == NOT_DETECTED || CurrentState == APPROACHING) {
            RemoteLog.log(ESP_LOG_VERBOSE, "esp_now", "Beacon is approaching.");
            CurrentState = APPROACHING;
        }
        else if (millis() - last_msg_ms < 32000 && CurrentState != APPROACHING) {
            RemoteLog.log(ESP_LOG_VERBOSE, "esp_now", "Beacon is detected.");
            CurrentState = DETECTED;
        }
        else {
            RemoteLog.log(ESP_LOG_VERBOSE, "esp_now", "Beacon is not detected.");
            CurrentState = NOT_DETECTED;
        }

        if (CurrentState == NEARBY && !opened) {
            Lock::UnlockDoor(15000);
            opened = true;
        }
        else if (CurrentState == NOT_DETECTED && opened)
        {
            opened = false;
        }

        Serial.println(String((int)CurrentState).c_str());
        esp_now_send(NULL, (uint8_t*) String(CurrentState).c_str(), 1);

        /*if (Lock::isLocking)
            esp_now_send(NULL, (uint8_t*)"locking", 9);
        else if (Lock::isUnlocking)
            esp_now_send(NULL, (uint8_t*)"unlocking", 9);
        else if (Lock::isEngaging)
            esp_now_send(NULL, (uint8_t*)"engaging", 9);
        else if (Lock::isDisengaging)
            esp_now_send(NULL, (uint8_t*)"disengaging", 9);
        else if (Lock::isLocked && Lock::isEngaged)
            esp_now_send(NULL, (uint8_t*)"locked", 6);
        else if (!Lock::isLocked && Lock::isEngaged)
            esp_now_send(NULL, (uint8_t*)"unlocked", 8);
        else if (!Lock::isEngaged)
            esp_now_send(NULL, (uint8_t*)"disengaged", 10);*/
    }
}