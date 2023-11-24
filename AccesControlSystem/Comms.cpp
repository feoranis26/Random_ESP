#include "Comms.h"

AsyncWebServer server(80);
FtpServer ftp_server;

void setupServer() {
    server.on("/action", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass") || !req->hasParam("action")) {
            req->send_P(400, "text/html", "Invalid request");
            return;
        }

        String key = req->getParam("pass")->value();
        if (!Authentication::Authenticate(key)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        String action = req->getParam("action")->value();

        String status;

        if (action == "unlock")
            status = Lock::UnlockDoor(30000);
        else if (action == "lock")
            status = Lock::LockDoor();
        else if (action == "engage")
            status = Lock::EngageDoor();
        else if (action == "disengage")
            status = Lock::DisengageDoor();
        else if (action == "reset")
            ESP.restart();
        else {
            req->send_P(400, "text/html", "Invalid action");
            return;
        }

        req->send_P(401, "application/json", ("{\"status\":\"" + status + "\"}").c_str());
        });

    server.on("/add_key", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass") || !req->hasParam("new_pass")) {
            req->send_P(400, "text/html", "Invalid request");
            return;
        }

        String key = req->getParam("pass")->value();
        if (!Authentication::Authenticate(key)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        String new_key = req->getParam("new_pass")->value();

        String status = Authentication::AddAuth(new_key);
        req->send_P(401, "application/json", ("{\"status\":\"" + status + "\"}").c_str());
        });

    server.on("/remove_key", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass") || !req->hasParam("remove_pass")) {
            req->send_P(400, "text/html", "Invalid request");
            return;
        }

        String key = req->getParam("pass")->value();
        if (!Authentication::Authenticate(key)) {
            req->send_P(401, "text/html", "Authentication failed");
            return;
        }

        String old_key = req->getParam("remove_pass")->value();

        String status = Authentication::RemoveAuth(old_key);
        req->send_P(401, "application/json", ("{\"status\":\"" + status + "\"}").c_str());
        });

    server.on("/reset", [](AsyncWebServerRequest* req) {
        if (!req->hasParam("pass")) {
            req->send_P(400, "text/html", "Invalid request");
            return;
        }

        req->send_P(401, "application/json", "{\"status\":\"ok\"}");

        delay(500);
        ESP.restart();
        });

    server.begin();
    ftp_server.begin("feoranis", "16777217");

    Serial1.begin(9600, SERIAL_8N1, 21, 22);

    xTaskCreate(wifi_alert_task, "wifi_alert", 4096, NULL, 0, NULL);
    xTaskCreate(ftp_task, "ftp_handler", 4096, NULL, 0, NULL);
    xTaskCreate(serial_task, "serial_handler", 4096, NULL, 0, NULL);
}

void wifi_alert_task(void* param) {
    bool connected = false;
    while (true) {
        if (!connected && WiFi.isConnected()) {
            Serial.println("IP: " + WiFi.localIP().toString());
            IO_fx::OnWiFiConnect();

            connected = true;
        }
        else if (connected && !WiFi.isConnected()) {
            connected = false;
        }

        delay(100);
    }
}

void ftp_task(void* param) {
    while (true) {
        ftp_server.handleFTP();
        delay(5);
    }
}

void serial_task(void* param) {
    String spliced[32];

    int wrong_pass_count = 0;
    long last_warning = 0;
    long last_entry = 0;

    bool last_locked = false;
    bool last_engaged = false;
    long last_update_ms;
    while (true) {
        if (Serial.available() || Serial1.available()) {
            Serial.println("available");
            int numSplices = 0;
            String read = "";
            HardwareSerial* receivedPort = Serial.available() ? &Serial : &Serial1;
            read = receivedPort->readStringUntil(';');

            int idx = read.indexOf(' ');
            while (idx != -1) {
                idx = read.indexOf(' ');

                String token = read.substring(0, idx);
                Serial.println(token);
                spliced[numSplices++] = token;
                read = read.substring(idx + 1);
            }

            if (spliced[0] == "wake") {
                IO_fx::OnKeypadWake();
                /*
                String status = "";

                if (!Lock::isEngaged)
                    status = "disengaged";
                else if (!Lock::isLocked)
                    status = "unlocked";
                else
                    status = "locked";

                receivedPort->println("status " + status + ";");*/
                continue;
            }

            if (millis() - last_warning < 10000) {
                receivedPort->println("status cooldown;");
                IO_fx::Warning();
                continue;
            }
            else if (millis() - last_entry < 2500) {
                IO_fx::Warning();
                receivedPort->println("status cooldown;");
            }
            if (!Authentication::Authenticate(spliced[0]))
            {
                wrong_pass_count++;
                last_entry = millis();

                if (wrong_pass_count < 4) {
                    IO_fx::OnKeypadWrongPassword();
                    receivedPort->println("status auth_err;");
                }
                else {
                    last_warning = millis();
                    IO_fx::Warning();
                    wrong_pass_count = 0;
                    receivedPort->println("status cooldown;");
                }
                continue;
            }

            wrong_pass_count = 0;

            if (numSplices == 2 && spliced[1] == "unlock") {
                String status = Lock::UnlockDoor(10000);
                Serial.println("Status: " + status);
                receivedPort->println("status " + status + ";");
            }
            else if (numSplices == 2 && spliced[1] == "lock") {
                String status = Lock::LockDoor();
                Serial.println("Status: " + status);
                receivedPort->println("status " + status + ";");
            }
            else if (numSplices == 2 && spliced[1] == "sw_engage") {
                String status = Lock::isEngaged ? Lock::DisengageDoor() : Lock::EngageDoor();

                Serial.println("Status: " + status);
                receivedPort->println("status " + status + ";");
            }
        }

        if (last_engaged != Lock::isEngaged || Lock::isEngaged && (Lock::isLocked != last_locked) || millis() - last_update_ms > 50000) {
            last_engaged = Lock::isEngaged;
            last_locked = Lock::isLocked;
            last_update_ms = millis();
            String status;
            if (!Lock::isEngaged)
                status = "disengaged";
            else if (!Lock::isLocked)
                status = "unlocked";
            else
                status = "locked";

            Serial1.println("update_status " + status + ";");
        }
        delay(50);
    }
}