#include <esp32-hal.h>
#include <esp32-hal-ledc.h>

void setup() {
    Serial.begin(115200);

    ledcSetup(0, 25000, 8);
    ledcAttachPin(21, 0);
}

void loop() {
    if (Serial.available()) {
        ledcWrite(0, atoi(Serial.readStringUntil('\n').c_str()));
    }

    double read = analogRead(4) / 4096.0;
    Serial.printf("Read: %f\n", read);
    ledcWrite(0, read * 256.0);

    delay(100);
}
