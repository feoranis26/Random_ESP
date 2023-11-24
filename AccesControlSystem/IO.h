#ifndef IO_h
#define IO_h

#include "fGUI.h"
#include "AccessControlGUI.h"
#include "Config.h"

#define EncoderAPin 33
#define EncoderBPin 32
#define EncoderSwPin 15

#define RGB_R 27
#define RGB_G 26
#define RGB_B 25
#define IndicatorPin 4

#define ServoPin -1

#define DoorSwitchPin 35
#define OpenButtonPin 34

#define BuzzerPin 14

#define BuzzerLEDCChannel 2

#define RGB_R_Channel 3
#define RGB_G_Channel 4
#define RGB_B_Channel 5

class Lock;

class RGB_LED {
    static uint8_t R_color;
    static uint8_t G_color;
    static uint8_t B_color;

    static uint8_t R_color_off;
    static uint8_t G_color_off;
    static uint8_t B_color_off;

    static long R_period;
    static long R_active_period;
    static long G_period;
    static long G_active_period;
    static long B_period;
    static long B_active_period;

    static void update();
    static void task(void* param);

public:
    static void Set(uint8_t R, uint8_t G, uint8_t B);

    static void SetR(uint8_t on_color, uint8_t off_color, long period, long on_duration);
    static void SetG(uint8_t on_color, uint8_t off_color, long period, long on_duration);
    static void SetB(uint8_t on_color, uint8_t off_color, long period, long on_duration);

    static void Init();
};

class IO_fx {
    static void lock_fx(void* param);
    static void unlock_fx(void* param);
    static void engage_fx(void* param);
    static void disengage_fx(void* param);
    static void open_fx(void* param);
    static void close_fx(void* param);
    static void wifi_fx(void* param);
    static void ftp_fx(void* param);
    static void card_fx(void* param);
    static void wake_fx(void* param);
    static void wrong_pass_fx(void* param);
    static void warning_fx(void* param);

    static void startup_fx(void* param);

    static void update_led();
    static void update_buzzer();

    static void status_update_task(void* param);

public:
    static void OnLock();
    static void OnUnlock();
    static void OnEngage();
    static void OnDisengage();
    static void OnOpen();
    static void OnClose();
    static void OnCardScan(uint id, bool authorized);
    static void OnWiFiConnect();
    static void OnFTPAction();
    static void OnKeypadWake();
    static void OnKeypadWrongPassword();
    static void Alarm_Blocking();
    static void Warning();

    static void Init();

    static void UpdateData();
    static void ReadData();
    static bool beepWhenOpen;
};

#endif // !Lock_h
