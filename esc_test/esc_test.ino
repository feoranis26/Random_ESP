#include <ESP32Servo.h>
Servo esc;

double current_us = 1500;
double target_us = 1500;
double accel_us_per_s = 100;
// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    Serial.setTimeout(0.01);

    delay(5000);
    esc.attach(22);
    esc.writeMicroseconds(1500);
    delay(5000);
}

// the loop function runs over and over again until power down or reset
double last_ms = 0;
double timeSinceReachedTarget = 0;
void loop() {
    if (Serial.available()) {
        int us = Serial.parseInt();

        if (us == 0) {
            esc.writeMicroseconds(1500);
            current_us = 1500;
            target_us = 1500;
        }
        else {
            target_us = us;
        }
    }

    unsigned long long current_time = millis();
    double dT_seconds = ((double)(current_time - last_ms)) / 1000.0;
    last_ms = current_time;

    if (abs(current_us - target_us) > 10) {
        if (current_us > target_us)
            current_us -= dT_seconds * accel_us_per_s;
        else
            current_us += dT_seconds * accel_us_per_s;
    }
    else
        current_us = target_us;

    if (current_us == target_us)
        timeSinceReachedTarget += dT_seconds;
    else
        timeSinceReachedTarget = 0;

    if (timeSinceReachedTarget > 10 && target_us == 1900)
        target_us = 1100;
    else if (timeSinceReachedTarget > 10 && target_us == 1100)
        target_us = 1900;

    esc.writeMicroseconds(current_us);
    delay(10);
}
