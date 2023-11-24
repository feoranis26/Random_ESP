#ifndef Lock_h
#define Lock_h

#include <ESP32Servo.h>
#include <Arduino.h>
#include <MFRC522.h>
//#include <WiFiClientSecure.h>
//#include <HTTPClient.h>

#include "IO.h"
#include "Authentication.h"
#include "Config.h"

class Lock {
    static Servo servo;

    static void attachServo();
    static void detachServo();

    static void unlock_task(void* param);
    static void lock_task(void* param);
    static void engage_task(void* param);
    static void disengage_task(void* param);


    static void doorOpened();
    static void doorClosed();

    static void cardScanned();

    static void door_task(void* param);

    static MFRC522 rfid;
    static long lastScanTime;
    static bool doorHasBeenOpened;

public:
    static void init();

    static String DisengageDoor();
    static String EngageDoor();
    static String UnlockDoor(long timeUntilAutoLock = 5000);
    static String LockDoor();

    static bool canChangeState();

    static double GetTimeToClose();
    static void UpdateData();
    static void ReadData();

    static void cam_task(void* param);

    static bool isLocked;
    static bool isEngaged;
    static bool isOpen;

    static bool isLocking;
    static bool isUnlocking;
    static bool isEngaging;
    static bool isDisengaging;

    static bool shouldLock;
    static bool shouldUnlock;

    static bool shouldEngage;
    static bool shouldDisengage;

    static long doorOpenCloseMillis;
    static long doorUnlockMillis;
    static long doorLockMillis;
    static long timeToLock;

    static bool engageSolenoidWhenOpen;
    static float doorLockTimeAfterClose;
};

#endif // !Lock_h