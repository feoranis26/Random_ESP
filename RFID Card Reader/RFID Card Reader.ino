#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>

#include <espnow.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

//MFRC522 rfid(D8, D0);

bool doorEngaged = false;
bool doorLocked = false;
bool doorPendingEngage = true;
bool doorPendingDisengage = false;
bool doorPendingEngageRead = false;
bool doorEngageRequested = false;
bool doorOpen = false;
bool doorPendingUnlock = false;

bool addCardPending = false;

bool doorPreviouslyEngaged = false;

bool alarm;
bool alarmState;


bool doneRequest = false;

long doorOpenTime = 0;
long doorClosedTime = 0;

uint authCards[255];
int authCardAmount = 0;

Servo lockServo;

AsyncWebServer server(80);

uint8_t* auth_peers[256];
int num_peers = 0;

long last_broadcast_ms = 0;
long doorTimeUntilClose = 0;

long doorAlarmStartTime = 0;

bool resetRequested = false;

void callback(uint8_t* mac, uint8_t* d, uint8_t len) {
    Serial.println("Data received from: " + String(mac[0], 16) + ":" + String(mac[1], 16) + ":" + String(mac[2], 16) + ":" + String(mac[3], 16) + ":" + String(mac[4], 16) + ":" + String(mac[5], 16));

    String d_str;
    for (int i = 0; i < len; i++) {
        d_str += (char)d[i];
    }

    Serial.println("Data: " + d_str);

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

        doorPendingUnlock = true;
    }

    if (d_str == "disengage") {
        Serial.println("Disengage requested.");

        doorPendingDisengage = true;
    }

    if (d_str == "engage") {
        Serial.println("Engage requested.");

        doorEngageRequested = true;
    }
}

void add_peer(const uint8_t* mac) {
    Serial.println("Adding peer.");
    uint8_t* mac_ = (uint8_t*)malloc(6);
    memcpy(mac_, mac, 6);
    auth_peers[num_peers] = mac_;
    num_peers++;

    esp_now_add_peer((u8*)mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}

bool is_peer(const uint8_t* mac) {
    for (int i = 0; i < num_peers; i++) {
        if (memcmp(auth_peers[i], mac, 6) == 0)
            return true;
    }

    return false;
}

void broadcast(String message) {
    for (int i = 0; i < num_peers; i++)
        esp_now_send(auth_peers[i], (uint8_t*)message.c_str(), message.length());
}

void setup() {
    //flib_Startup();
    Serial.begin(115200);
    Serial.println("Starting up...");

    //SPI.begin();
    //rfid.PCD_Init();

    lockServo.attach(D1, 1000, 2750);
    lockServo.write(0);

    //byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
    //Serial.print(F("MFRC522 Software Version: 0x"));
    //Serial.println(v, HEX);

    Serial.println("Ok");

    pinMode(2, OUTPUT);
    //pinMode(9, OUTPUT);
    pinMode(D2, INPUT);
    digitalWrite(D2, HIGH);

    pinMode(10, INPUT);
    digitalWrite(10, HIGH);

    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    pinMode(D6, OUTPUT);
    digitalWrite(D6, HIGH);

    pinMode(D5, OUTPUT);
    digitalWrite(D5, LOW);

    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname("frc-lock");
    WiFi.begin("BK_School", "8K-$cH0oL!");
    //WiFi.begin("S.A.A.D-2.4G_TV", "04Temmuz2003");


    Serial.println("MAC: " + WiFi.macAddress());

    while (!WiFi.isConnected()) {
        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);
        delay(1000);
    }
    Serial.println("Connected to wifi! IP is: " + WiFi.localIP().toString());
    Serial.println("Channel: " + String(WiFi.channel()));

    tone(D3, 440);
    delay(250);
    tone(D3, 660);
    delay(250);
    tone(D3, 880);
    delay(375);
    noTone(D3);

    if (esp_now_init() != 0)
        Serial.println("Can't init ESP-NOW");

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(callback);


    server.on("/unlock", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }


        if (doorEngaged && doorLocked) {
            doorPendingUnlock = true;
            req->send_P(200, "text/html", "ok");
        }
        else if (!doorEngaged)
            req->send_P(500, "text/html", "error:door_not_engaged");
        else if (!doorLocked)
            req->send_P(500, "text/html", "error:door_not_locked");

        });

    server.on("/disengage", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        if (doorEngaged) {
            doorPendingDisengage = true;
            req->send_P(200, "text/html", "ok");
        }
        else
            req->send_P(500, "text/html", "error:door_not_engaged");

        });

    server.on("/engage", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        if (doorEngaged) {
            req->send_P(500, "text/html", "error:door_not_disengaged");
            return;
        }

        doorEngageRequested = true;

        if (doorOpen) {
            doorLocked = false;
            doorOpenTime = millis();
        }

        long start_t = millis();
        doneRequest = false;

        AsyncWebServerResponse* resp = req->beginChunkedResponse("text/html", [start_t](uint8_t* buffer, size_t max_len, size_t index) -> size_t {
            if (doneRequest) {
                doneRequest = false;
                return 0;
            }

            if ((millis() - start_t) > 15000) {
                String("warning:door_open").toCharArray((char*)buffer, max_len);

                doneRequest = true;
                return 21;
            }

            if (doorEngaged && doorLocked) {
                String("ok").toCharArray((char*)buffer, max_len);
                doneRequest = true;
                return 2;
            }

            String(" ").toCharArray((char*)buffer, max_len);
            return 1;
            });

        req->send(resp);

        });

    server.on("/reset_cards", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        authCardAmount = 0;
        SaveCards();

        req->send_P(200, "text/html", "ok");
        });

    server.on("/add_card", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        long long nuid = atoll(req->getParam("nuid")->value().c_str());
        authCards[authCardAmount] = nuid;
        authCardAmount++;

        SaveCards();

        Serial.println("Added card with NUID:");
        Serial.println(nuid, HEX);

        req->send_P(200, "text/html", "ok");
        });
    /*
    server.on("/read_and_add_card", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        addCardPending = true;

        req->send_P(200, "text/html", "ok");
        });
        */
    server.on("/read_and_add_card", [](AsyncWebServerRequest* r) {
        if (!CheckAccess(r)) {
            r->send_P(401, "text/html", "Authentication failed");
            return;
        }

        long start_t = millis();
        addCardPending = true;
        Serial.println("start q");
        doneRequest = false;

        AsyncWebServerResponse* resp = r->beginChunkedResponse("text/html", [start_t](uint8_t* buffer, size_t max_len, size_t index) -> size_t {
            if ((millis() - start_t) > 5000 && !doneRequest) {
                String("error:no_card_scanned").toCharArray((char*)buffer, max_len);
                doneRequest = true;
                return 21;
            }

            if (!addCardPending) {
                String("ok").toCharArray((char*)buffer, max_len);
                doneRequest = true;
                return 2;
            }

            if (doneRequest) {
                doneRequest = false;
                return 0;
            }

            String(" ").toCharArray((char*)buffer, max_len);
            return 1;
            });
        r->send(resp);
        });

    server.on("/get_cards", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        DynamicJsonDocument d(1024);
        JsonArray a = d.createNestedArray("cards");

        for (int i = 0; i < authCardAmount; i++) {
            uint cardNUID = authCards[i];
            a.add(cardNUID);
        }

        String d_str;
        serializeJson(d, d_str);

        req->send_P(200, "application/json", d_str.c_str());
        });

    server.on("/reset", [](AsyncWebServerRequest* req) {
        if (!CheckAccess(req)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }
        resetRequested = true;
        req->send_P(200, "text/html", "ok");
        });
    /*
    server.on("/disengage", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass"))
            req->send(400);

        if (req->getParam("pass")->value() != "16777216")
            req->send(403, "Authentication failed");

        doorEngaged = false;

        req->send(200, "ok");
        });

    server.on("/engage", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass"))
            req->send(400);

        if (req->getParam("pass")->value() != "16777216")
            req->send(403, "Authentication failed");

        doorPendingEngage = true;

        req->send(200, "ok");
        });
    /*
    server.on("/add_card", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass") || !req->hasParam("nuid"))
            req->send(400);

        if (req->getParam("pass")->value() != "16777216")
            req->send(403, "Authentication failed");

        long long nuid = atoll(req->getParam("nuid")->value().c_str());
        authCards[authCardAmount] = nuid;
        authCardAmount++;

        //SaveCards();

        Serial.println("Added card with NUID:");
        Serial.println(nuid, HEX);

        req->send(200, "ok");
        });

    server.on("/reset_cards", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass") || !req->hasParam("nuid"))
            req->send(400);

        if (req->getParam("pass")->value() != "16777216")
            req->send(403, "Authentication failed");

        int err_code = LittleFS.remove("/cards.dat") ? 200 : 500;

        req->send(err_code, "ok");
        });


    */

    LoadCards();
    server.begin();
    ArduinoOTA.begin("FRC_Door_8266");
}

void LoadCards() {
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS failed to begin!");
        return;
    }

    File cardsFile = LittleFS.open("/cards.json", "r");
    String readall = cardsFile.readString();
    Serial.println("Read card data: " + readall);
    cardsFile.close();

    DynamicJsonDocument d(1024);
    deserializeJson(d, readall);
    Serial.println("deserialized");

    for (uint nuid : d["cards"].as<JsonArray>()) {
        Serial.println("Read");
        authCards[authCardAmount] = nuid;
        authCardAmount++;

        //Serial.println("Read card: " + String(nuid));
    }

    authCards[authCardAmount] = (uint)16777216;
    authCardAmount++;

    Serial.println("Card amount : " + String(authCardAmount));
}

void SaveCards() {
    DynamicJsonDocument d(1024);
    JsonArray a = d.createNestedArray("cards");

    for (int i = 0; i < authCardAmount; i++) {
        uint cardNUID = authCards[i];

        if (cardNUID != 0016777216)
            a.add(cardNUID);
    }

    String d_str;
    serializeJson(d, d_str);

    File cardsFile = LittleFS.open("/cards.json", "w");
    cardsFile.print(d_str);
    cardsFile.close();
}

bool CheckAccess(AsyncWebServerRequest* req) {
    String pass;

    if (req->hasHeader("pass"))
        pass = req->getHeader("pass")->value();
    else if (req->hasParam("pass"))
        pass = req->getParam("pass")->value();

    Serial.println("Authenticating: " + pass);

    uint nuid = atoll(pass.c_str());

    if (nuid == 0)
        return false;

    for (int i = 0; i < authCardAmount; i++) {
        if (nuid == authCards[i]) {
            return true;
        }
    }

    return false;
}

void ReadCard() {
    //Serial.println("Waiting for card data...");
    //while (!rfid.PICC_ReadCardSerial())
        //delay(10);
    Serial.println("Card read.");
    //rfid.PICC_ReadCardSerial();
    delay(100);

    uint nuid;
    for (byte i = 0; i < 4; i++) {
        //((byte*)&nuid)[i] = rfid.uid.uidByte[i];
    }
    Serial.println("NUID is: ");
    Serial.println(nuid, HEX);

    for (int i = 0; i < authCardAmount; i++) {
        if (nuid == authCards[i]) {
            OnAuthCardRead();
            return;
        }
    }

    if (addCardPending || (!doorLocked && doorEngaged && !digitalRead(10))) {
        authCards[authCardAmount] = nuid;
        authCardAmount++;

        addCardPending = false;

        tone(D3, 1000);
        delay(250);
        tone(D3, 2000);
        delay(250);
        tone(D3, 3000);
        delay(250);
        noTone(D3);

        SaveCards();

        delay(1000);

        return;
    }

    tone(D3, 1500);
    delay(500);
    tone(D3, 1000);
    delay(500);
    noTone(D3);
}

void OnAuthCardRead() {
    if (doorEngaged)
        UnlockDoor();
    else if (doorPendingEngage && !doorPendingEngageRead)
        doorPendingEngageRead = true;
}

void UnlockDoor() {
    if (!doorEngaged)
        return;

    Serial.println("Unlocking door!");
    doorLocked = false;
    doorOpenTime = millis();
    doorTimeUntilClose = 30000;

    digitalWrite(D7, LOW);
    digitalWrite(D6, HIGH);
    digitalWrite(D5, LOW);

    tone(D3, 1000);
    delay(500);

    digitalWrite(D7, HIGH);
    digitalWrite(D6, LOW);
    digitalWrite(D5, HIGH);
}

void loop() {
    ArduinoOTA.handle();

    if (resetRequested) {
        tone(D3, 2000);
        delay(250);
        tone(D3, 1800);
        delay(250);
        tone(D3, 1600);
        delay(250);
        tone(D3, 1400);
        delay(250);
        tone(D3, 1200);
        delay(500);

        ESP.restart();
    }

    if (!doorLocked) {
        if (!doorOpen && digitalRead(D2)) {
            doorOpen = true;
            tone(D3, 1000);
            delay(250);
            tone(D3, 1500);
            delay(250);
        }
        else if (doorOpen && !digitalRead(D2)) {
            doorOpen = false;
            tone(D3, 1250);
            delay(250);
            tone(D3, 750);
            delay(250);
        }

        if (doorEngaged) {
            if (doorTimeUntilClose - (millis() - doorOpenTime) > 4500) {
                if ((millis() % 2000) < 200)
                    tone(D3, 600);
                else
                    noTone(D3);
            }
            else if (doorTimeUntilClose - (millis() - doorOpenTime) > 2500) {
                if ((millis() % 1000) < 200)
                    tone(D3, 600);
                else
                    noTone(D3);
            }
            else {
                if ((millis() % 500) < 200)
                    tone(D3, 1000);
                else
                    noTone(D3);
            }
        }
    }
    else
        doorOpen = digitalRead(D2);

    if (doorOpen && doorEngaged)
        doorTimeUntilClose = 5000;

    /*if (rfid.PICC_IsNewCardPresent()) {
        Serial.println("[Main] New card present!");
        ReadCard();
    }*/

    if (doorPendingUnlock) {
        if (doorEngaged && doorLocked) {
            broadcast("unlocking");
            UnlockDoor();
            broadcast("unlocked");
        }
        else if (!doorEngaged)
            broadcast("disengaged");
        else if (!doorLocked)
            broadcast("unlocked");

        doorPendingUnlock = false;
    }

    if (doorPendingDisengage)
    {
        if (doorEngaged) {
            doorEngaged = false;
        }
        else
            broadcast("disengaged");

        doorPendingDisengage = false;
    }

    if (doorEngageRequested) {
        if (!doorEngaged) {
            broadcast("engaging");

            doorEngaged = true;

            //if (doorOpen) {
            doorLocked = false;
            doorOpenTime = millis();
            doorTimeUntilClose = doorOpen ? 5000 : 0;
            //}
        }
        else
            broadcast("engaged");

        doorEngageRequested = false;
    }

    if (millis() - last_broadcast_ms > 1000) {
        if (doorLocked && doorEngaged)
            esp_now_send(NULL, (uint8_t*)"locked", 6);
        else if (!doorLocked && doorEngaged)
            esp_now_send(NULL, (uint8_t*)"unlocked", 8);
        else if (!doorEngaged)
            esp_now_send(NULL, (uint8_t*)"disengaged", 10);

        last_broadcast_ms = millis();
    }

    if (doorEngaged && !doorLocked && millis() - doorOpenTime > doorTimeUntilClose && !doorOpen) {
        broadcast("locking");

        digitalWrite(D7, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D5, LOW);

        doorLocked = true;
        lockServo.write(100);
        tone(D3, 2000);
        delay(1000);
        lockServo.write(90);

        tone(D3, 1000);
        delay(750);
        lockServo.detach();
        tone(D3, 1250);
        delay(250);

        digitalWrite(D7, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D5, HIGH);

        broadcast("locked");
    }

    if (doorEngaged) {
        if (!digitalRead(10)) {
            UnlockDoor();
            doorTimeUntilClose = 5000;
        }

        lockServo.write(doorLocked ? 90 : 0);
    }

    if (doorEngaged) {
        if (!doorLocked) {
            if (doorTimeUntilClose - (millis() - doorOpenTime) > 4500)
                digitalWrite(2, (millis() % 500) < 100 ? LOW : HIGH);
            else if (doorTimeUntilClose - (millis() - doorOpenTime) > 2500)
                digitalWrite(2, (millis() % 250) < 100 ? LOW : HIGH);
            else
                digitalWrite(2, (millis() % 125) < 62 ? LOW : HIGH);

            if(!lockServo.attached())
                lockServo.attach(D1, 1000, 2750);
        }
        else {
            digitalWrite(2, (millis() % 1000) < 500 ? HIGH : LOW);

            if (!alarm)
                noTone(D3);

            if (lockServo.attached())
                lockServo.detach();
        }
    }
    else {
        if (doorPendingEngage && doorPendingEngageRead) {

            if (!doorEngaged) {
                tone(D3, 1000);
                delay(500);
            }

            doorEngageRequested = true;
        }

        if (doorPendingEngageRead && !doorOpen) {
            doorEngaged = true;
            doorPendingEngage = false;
            doorPendingEngageRead = false;
        }

        if (doorPendingEngageRead)
            digitalWrite(2, LOW);
        else if (doorPendingEngage)
            digitalWrite(2, (millis() % 500) < 250 ? LOW : HIGH);
        else
            digitalWrite(2, HIGH);

        digitalWrite(10, HIGH);

        lockServo.write(0);
    }

    if (doorPreviouslyEngaged && !doorEngaged) {
        digitalWrite(D7, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D5, LOW);
        tone(D3, 1750);
        delay(250);
        digitalWrite(D7, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D5, LOW);
        tone(D3, 1200);
        delay(250);
        digitalWrite(D7, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D5, HIGH);
        tone(D3, 750);
        delay(500);
        noTone(D3);

        lockServo.attach(D1, 1000, 2750);
        doorPreviouslyEngaged = false;
        broadcast("disengaged");
    }
    else if (!doorPreviouslyEngaged && doorEngaged) {

        digitalWrite(D7, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D5, LOW);

        tone(D3, 1750);
        delay(250);
        tone(D3, 1000);
        delay(250);
        noTone(D3);
        digitalWrite(D7, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D5, LOW);
        delay(250);
        digitalWrite(D7, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D5, LOW);
        tone(D3, 1000);
        delay(250);
        noTone(D3);
        delay(250);
        tone(D3, 1000);
        delay(250);
        digitalWrite(D7, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D5, HIGH);
        noTone(D3);

        doorPreviouslyEngaged = true;
        broadcast("engaged");
    }

    bool a_s = millis() % 500 > 250;
    if (alarm && alarmState != a_s) {
        alarmState = a_s;
        tone(D3, a_s ? 1000 : 750);
    }

    if (digitalRead(D2))
        doorOpenTime = millis();
    else
        doorClosedTime = millis();

    if (!doorEngaged)
        noTone(D3);

    if (doorEngaged && doorLocked && digitalRead(D2) && !alarm)
    {
        tone(D3, 2500);
        digitalWrite(2, HIGH);

        if (millis() - doorClosedTime > 2000)
            alarm = true;
    }

    if (doorEngaged) {
        if (doorLocked) {
            digitalWrite(D7, LOW);
            digitalWrite(D6, HIGH);
            digitalWrite(D5, HIGH);
        }
        else if(!doorOpen) {
            if (doorTimeUntilClose - (millis() - doorOpenTime) > 4500) {
                digitalWrite(D7, HIGH);
                digitalWrite(D6, LOW);
                digitalWrite(D5, HIGH);
            }
            else if (doorTimeUntilClose - (millis() - doorOpenTime) > 2500) {
                digitalWrite(D7, LOW);
                digitalWrite(D6, millis() % 1000 > 500);
                digitalWrite(D5, HIGH);
            }
            else {
                digitalWrite(D7, LOW);
                digitalWrite(D6, HIGH);
                digitalWrite(D5, HIGH);
            }
        }
        else {
            digitalWrite(D7, millis() % 5000 > 100);
            digitalWrite(D6, LOW);
            digitalWrite(D5, millis() % 5000 > 100);
        }
    }
    else {
        if (doorOpen) {
            digitalWrite(D7, HIGH);
            digitalWrite(D6, LOW);
            digitalWrite(D5, HIGH);
        }
        else {
            digitalWrite(D7, LOW);
            digitalWrite(D6, LOW);
            digitalWrite(D5, HIGH);
        }
    }

    delay(20);
}
