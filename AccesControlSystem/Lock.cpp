#include "Lock.h"

Servo Lock::servo;

bool Lock::isLocked = false;
bool Lock::isEngaged = false;
bool Lock::isOpen = false;

bool Lock::isLocking = false;
bool Lock::isUnlocking = false;
bool Lock::isEngaging = false;
bool Lock::isDisengaging = false;

bool Lock::shouldLock = false;
bool Lock::shouldUnlock = false;

bool Lock::shouldEngage = false;
bool Lock::shouldDisengage = false;

bool Lock::doorHasBeenOpened = false;

long Lock::doorOpenCloseMillis = 0;
long Lock::doorUnlockMillis = 0;
long Lock::doorLockMillis = 0;
long Lock::timeToLock = 0;

long Lock::lastScanTime = 0;

bool Lock::engageSolenoidWhenOpen = false;
float Lock::doorLockTimeAfterClose = 0;


MFRC522 Lock::rfid(5, 0);

void Lock::attachServo() {
    servo.attach(ServoPin, 1000, 2750);
}

void Lock::detachServo() {
    servo.detach();
}

void Lock::unlock_task(void* param) {
    isUnlocking = true;
    IO_fx::OnUnlock();

    digitalWrite(12, HIGH);
    delay(50);
    digitalWrite(12, LOW);
    delay(50);
    digitalWrite(12, HIGH);
    delay(250);
    digitalWrite(12, LOW);
    delay(50);

    attachServo();
    servo.write(0);

    doorUnlockMillis = millis();
    isUnlocking = false;
    isLocked = false;
    doorHasBeenOpened = false;
    vTaskDelete(NULL);
    delay(0);
}

void Lock::lock_task(void* param) {
    isLocking = true;
    IO_fx::OnLock();
    Serial.println("Locking door.");

    attachServo();
    servo.write(100);
    delay(1000);
    servo.write(90);
    delay(1000);
    detachServo();


    doorLockMillis = millis();
    isLocking = false;
    isLocked = true;
    vTaskDelete(NULL);
    delay(0);
}

void Lock::engage_task(void* param) {
    isEngaging = true;
    IO_fx::OnEngage();
    Serial.println("Engaging door.");

    if (!isOpen) {
        attachServo();
        servo.write(100);
        delay(1000);
        servo.write(90);
        delay(1000);
        detachServo();

        isLocked = true;
    }
    else {
        isLocked = false;
        timeToLock = 5000;
    }

    isEngaging = false;
    isEngaged = true;
    vTaskDelete(NULL);
    delay(0);
}

void Lock::disengage_task(void* param) {
    isDisengaging = true;
    IO_fx::OnDisengage();

    attachServo();
    servo.write(0);
    delay(1000);

    isDisengaging = false;
    isEngaged = false;
    vTaskDelete(NULL);
    delay(0);
}


void Lock::doorOpened() {
    Serial.println("Door opened");
    IO_fx::OnOpen();

    isOpen = true;
    doorHasBeenOpened = true;
    doorOpenCloseMillis = millis();
}

void Lock::doorClosed() {
    Serial.println("Door closed");

    if(!isEngaged || doorLockTimeAfterClose >= 2)
        IO_fx::OnClose();

    isOpen = false;
    doorOpenCloseMillis = millis();

    timeToLock = doorLockTimeAfterClose * 1000;
}

void Lock::cardScanned() {
    if (millis() - lastScanTime < 10000)
        return;

    lastScanTime = millis();

    Serial.println("Card read.");
    rfid.PICC_ReadCardSerial();
    delay(100);

    uint nuid;
    for (byte i = 0; i < 4; i++) {
        ((byte*)&nuid)[i] = rfid.uid.uidByte[i];
    }

    Serial.println("Card ID: " + String((unsigned long)nuid, 16));
    bool authenticated = Authentication::Authenticate(String(nuid, 16));

    if (authenticated) {
        Serial.println("Card is authenticated");
        UnlockDoor(10000);
    }


    IO_fx::OnCardScan(nuid, authenticated);
}

void Lock::door_task(void* param) {
    long last_check_ms = 0;
    while (true) {
        if (shouldLock) {
            shouldLock = false;
            xTaskCreate(lock_task, "lock", 4096, NULL, 0, NULL);
        }

        if (shouldUnlock) {
            shouldUnlock = false;
            xTaskCreate(unlock_task, "unlock", 4096, NULL, 0, NULL);
        }

        if (shouldEngage) {
            shouldEngage = false;
            xTaskCreate(engage_task, "engage", 4096, NULL, 0, NULL);
        }

        if (shouldDisengage) {
            shouldDisengage = false;
            xTaskCreate(disengage_task, "disengage", 4096, NULL, 0, NULL);
        }

        bool doorSwitchState = digitalRead(DoorSwitchPin);

        if (doorSwitchState && !isOpen)
            doorOpened();
        else if (!doorSwitchState && isOpen)
            doorClosed();

        bool doorButtonState = !digitalRead(OpenButtonPin);
        if (doorButtonState && isLocked && !isUnlocking)
            UnlockDoor();

        if (doorButtonState)
            fGUI::ResetScreensaver();

        if (isEngaged && !isOpen && !isLocked) {
            if (millis() - max(doorOpenCloseMillis, doorUnlockMillis) > timeToLock || (doorHasBeenOpened && doorLockTimeAfterClose == 0.0))
                LockDoor();
        }
        if (!isUnlocking) {
            if (!fGUI::IsScreensaverActive() && (!isEngaged || (isEngaged && !isLocked && !isLocking && !(doorHasBeenOpened && !(engageSolenoidWhenOpen && millis() - doorOpenCloseMillis > 50)))))
                digitalWrite(12, HIGH);
            else
                digitalWrite(12, LOW);
        }


        if (millis() - last_check_ms > 250) {
            last_check_ms = millis();
            if (rfid.PICC_IsNewCardPresent())
                cardScanned();
        }

        if (isEngaged && isLocked && isOpen && !shouldUnlock && !isLocking && !isUnlocking)
        {
            if (millis() - doorLockMillis < 5000) {
                /*digitalWrite(14, HIGH);
                delay(100);
                digitalWrite(14, LOW);*/

                ledcWriteTone(BuzzerLEDCChannel, 3000);
                delay(100);
                ledcWrite(BuzzerLEDCChannel, 0);

                UnlockDoor();
            }
            else {
                xTaskCreate(cam_task, "cam_http", 4096, NULL, 0, NULL);
                IO_fx::Alarm_Blocking();
                ESP.restart();
            }
        }

        delay(10);
    }
}

void Lock::init()
{
    attachServo();
    servo.write(0);

    pinMode(12, OUTPUT);

    pinMode(DoorSwitchPin, INPUT_PULLUP);
    pinMode(OpenButtonPin, INPUT_PULLUP);

    xTaskCreate(door_task, "door", 8192, NULL, 0, NULL);

    rfid.PCD_Init();
    byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
    Serial.print("MFRC522 Software Version : 0x" + String((long)HEX, 16));

    EngageDoor();
}

String Lock::DisengageDoor() {
    if (!isEngaged)
        return "not_engaged";

    if (!canChangeState())
        return "busy";

    shouldDisengage = true;
    return "ok";
}

String Lock::EngageDoor() {
    if (isEngaged)
        return "not_disengaged";

    if (!canChangeState())
        return "busy";

    shouldEngage = true;
    return "ok";
}

String Lock::UnlockDoor(long timeUntilAutoLock/* = 5000 */ ) {
    if (!isEngaged)
        return "not_engaged";

    if (!isLocked)
        return "not_locked";

    if (!canChangeState())
        return "busy";

    shouldUnlock = true;
    timeToLock = timeUntilAutoLock;
    xTaskCreate(cam_task, "cam_http", 4096, NULL, 0, NULL);
    return "ok";
}

String Lock::LockDoor() {
    if (!isEngaged)
        return "not_engaged";

    if (isLocked)
        return "not_unlocked";

    if (!canChangeState())
        return "busy";

    shouldLock = true;
    return "ok";
}

bool Lock::canChangeState() {
    return !(isUnlocking || isLocking || isEngaging || isDisengaging);
}

double Lock::GetTimeToClose() {
    if (isOpen)
        return MAXFLOAT;
    return (double)(timeToLock - (millis() - max(doorOpenCloseMillis, doorUnlockMillis))) / 1000.0;
}

void Lock::UpdateData()
{
    Config::Object["engageSolenoidWhenOpen"] = engageSolenoidWhenOpen;
    Config::Object["doorLockTimeAfterClose"] = doorLockTimeAfterClose;
}

void Lock::ReadData()
{
    engageSolenoidWhenOpen = Config::Object["engageSolenoidWhenOpen"];
    doorLockTimeAfterClose = Config::Object["doorLockTimeAfterClose"];
}

void Lock::cam_task(void* param)
{
    /*
    HTTPClient http;
    http.begin("http://10.134.102.227/capture");
    http.GET();
    */
    vTaskDelete(NULL);
    delay(0);
}
