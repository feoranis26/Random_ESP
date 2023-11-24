#include "IO.h"

uint8_t RGB_LED::R_color;
uint8_t RGB_LED::G_color;
uint8_t RGB_LED::B_color;

uint8_t RGB_LED::R_color_off;
uint8_t RGB_LED::G_color_off;
uint8_t RGB_LED::B_color_off;

long RGB_LED::R_period;
long RGB_LED::R_active_period;
long RGB_LED::G_period;
long RGB_LED::G_active_period;
long RGB_LED::B_period;
long RGB_LED::B_active_period;

bool io_led_update = true;

bool IO_fx::beepWhenOpen = false;

void RGB_LED::update() {
    long m = millis();
    bool R_on = m % R_period < R_active_period;
    bool G_on = m % G_period < G_active_period;
    bool B_on = m % B_period < B_active_period;

    uint8_t R_duty = R_on ? R_color : R_color_off;
    uint8_t G_duty = G_on ? G_color : G_color_off;
    uint8_t B_duty = B_on ? B_color : B_color_off;

    ledcWrite(RGB_R_Channel, R_duty);
    ledcWrite(RGB_G_Channel, G_duty);
    ledcWrite(RGB_B_Channel, B_duty);
}

void RGB_LED::task(void* param) {
    while (true) {
        update();
        delay(20);
    }
}

void RGB_LED::Set(uint8_t R, uint8_t G, uint8_t B) {
    R_color = R;
    G_color = G;
    B_color = B;

    R_color_off = R;
    G_color_off = G;
    B_color_off = B;

    R_period = 1;
    R_active_period = 0;
    G_period = 1;
    G_active_period = 0;
    B_period = 1;
    B_active_period = 0;
}

void RGB_LED::SetR(uint8_t on_color, uint8_t off_color, long period, long on_duration) {
    R_color = on_color;
    R_color_off = off_color;
    R_period = period;
    R_active_period = on_duration;
}

void RGB_LED::SetG(uint8_t on_color, uint8_t off_color, long period, long on_duration) {
    G_color = on_color;
    G_color_off = off_color;
    G_period = period;
    G_active_period = on_duration;
}

void RGB_LED::SetB(uint8_t on_color, uint8_t off_color, long period, long on_duration) {
    B_color = on_color;
    B_color_off = off_color;
    B_period = period;
    B_active_period = on_duration;
}

void RGB_LED::Init() {
    ledcSetup(RGB_R_Channel, 5000, 8);
    ledcSetup(RGB_G_Channel, 5000, 8);
    ledcSetup(RGB_B_Channel, 5000, 8);

    ledcAttachPin(RGB_R, RGB_R_Channel);
    ledcAttachPin(RGB_G, RGB_G_Channel);
    ledcAttachPin(RGB_B, RGB_B_Channel);

    xTaskCreate(task, "led_task", 4096, NULL, 0, NULL);
}

void IO_fx::lock_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(1000);
    ledcWriteTone(BuzzerLEDCChannel, 750);
    delay(200);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::unlock_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 2000);
    delay(150);
    ledcWriteTone(BuzzerLEDCChannel, 1000);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(200);
    ledcWriteTone(BuzzerLEDCChannel, 1000);
    delay(200);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::engage_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(50);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(50);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(125);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::disengage_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1000);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 750);
    delay(150);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::open_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1000);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::close_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1250);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::wifi_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1250);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::ftp_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1250);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::wake_fx(void* param) {
    io_led_update = false;
    RGB_LED::Set(255, 255, 255);

    ledcWriteTone(BuzzerLEDCChannel, 1250);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    RGB_LED::Set(255, 0, 0);
    io_led_update = true;

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::wrong_pass_fx(void* param) {
    io_led_update = false;
    RGB_LED::Set(0, 0, 0);

    ledcWriteTone(BuzzerLEDCChannel, 2000);
    delay(250);
    RGB_LED::Set(255, 0, 0);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(250);
    RGB_LED::Set(0, 0, 0);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(250);
    RGB_LED::Set(255, 0, 0);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(250);
    RGB_LED::Set(0, 0, 0);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(250);
    RGB_LED::Set(255, 0, 0);
    io_led_update = true;

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::startup_fx(void* param) {
    /*digitalWrite(14, HIGH);
    delay(500);
    digitalWrite(14, LOW);
    delay(100);*/

    ledcWriteTone(BuzzerLEDCChannel, 1750);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 2500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);
    delay(100);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::card_fx(void* param) {
    ledcWriteTone(BuzzerLEDCChannel, 1250);
    delay(100);
    ledcWriteTone(BuzzerLEDCChannel, 1500);
    delay(100);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::warning_fx(void* param) {
    /*digitalWrite(14, HIGH);
    delay(500);
    digitalWrite(14, LOW);*/

    ledcWriteTone(BuzzerLEDCChannel, 3000);
    delay(500);
    ledcWrite(BuzzerLEDCChannel, 0);

    vTaskDelete(NULL);
    delay(0);
}

void IO_fx::update_led() {
    if (!io_led_update)
        return;

    if (!Lock::canChangeState()) {
        RGB_LED::Set(255, 8, 0);
        digitalWrite(IndicatorPin, millis() % 250 < 125);
        return;
    }

    if (Lock::isEngaged) {
        if (Lock::isLocked) {
            RGB_LED::Set(255, 0, 0);
            digitalWrite(IndicatorPin, millis() % 1000 < 500);
        }
        else if (Lock::GetTimeToClose() > 2)
        {
            if (!Lock::isOpen) {
                RGB_LED::Set(0, 2550, 0);
                digitalWrite(IndicatorPin, millis() % 500 < 400);
            }
            else
            {
                RGB_LED::SetG(8, 255, 2000, 250);
                RGB_LED::SetR(255, 0, 2000, 250);
                digitalWrite(IndicatorPin, millis() % 500 < 250);
            }
        }
        else {
            digitalWrite(IndicatorPin, millis() % 250 < 125);
            RGB_LED::Set(255, 8, 0);
        }
    }
    else {
        digitalWrite(IndicatorPin, millis() % 5000 < 2500);
        RGB_LED::Set(255, 8, 0);
    }

}
int last_buzzer_state;
void IO_fx::update_buzzer() {
    if (beepWhenOpen && Lock::isEngaged && !Lock::isLocked && !Lock::isLocking) {
        int next_buzzer_state = 0;

        if (Lock::GetTimeToClose() < 2.5)
            next_buzzer_state = (millis() % 500 < 250) ? 2000 : 0;
        else
            next_buzzer_state = (millis() % 2000 < 250) ? 750 : 0;

        if (last_buzzer_state != next_buzzer_state)
            ledcWriteTone(BuzzerLEDCChannel, next_buzzer_state);

        last_buzzer_state = next_buzzer_state;
    }
}

void IO_fx::status_update_task(void* param) {
    while (true) {
        update_led();
        update_buzzer();
        delay(50);
    }
}

void IO_fx::OnLock() {
    xTaskCreate(lock_fx, "lock_fx", 4096, NULL, 0, NULL);

    if (fGUI::CurrentOpenMenu == 6)
        fGUI::ExitMenu();

    fGUI::OpenMenu(5);
    fGUI::ResetScreensaver();
}

void IO_fx::OnUnlock() {
    xTaskCreate(unlock_fx, "unlock_fx", 4096, NULL, 0, NULL);
    fGUI::OpenMenu(6);
    fGUI::ResetScreensaver();
}

void IO_fx::OnEngage() {
    xTaskCreate(engage_fx, "engage_fx", 4096, NULL, 0, NULL);
    fGUI::OpenMenu(1);
    fGUI::ResetScreensaver();
}

void IO_fx::OnDisengage() {
    xTaskCreate(disengage_fx, "disengage_fx", 4096, NULL, 0, NULL);
    fGUI::OpenMenu(2);
    fGUI::ResetScreensaver();
}

void IO_fx::OnOpen() {
    if (millis() < 5000)
        return;

    if (fGUI::CurrentOpenMenu == 4)
        fGUI::ExitMenu();

    if (fGUI::CurrentOpenMenu != 3)
        fGUI::OpenMenu(3);

    xTaskCreate(open_fx, "open_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::OnClose() {
    if (millis() < 5000)
        return;

    if (fGUI::CurrentOpenMenu == 3)
        fGUI::ExitMenu();

    if (fGUI::CurrentOpenMenu != 4)
        fGUI::OpenMenu(4);

    xTaskCreate(close_fx, "close_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::OnCardScan(uint id, bool authorized) {
    if (fGUI::CurrentOpenMenu != 7 && fGUI::CurrentOpenMenu != 9)
    {
        fGUI::OpenMenu(7);

        CardScannedMenu::cardID->t = String(id, 16) + (authorized ? " Card OK" : " Card unknown.");
        CardScannedMenu::bg_col = authorized ? TFT_GREEN : TFT_RED;
        CardScannedMenu::nuidString = String(id, 16);
    }

    xTaskCreate(card_fx, "card_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::OnWiFiConnect() {
    fGUI::OpenMenu(8);
    xTaskCreate(wifi_fx, "wifi_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::OnFTPAction() {
}

void IO_fx::OnKeypadWake() {
    xTaskCreate(wake_fx, "wake_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::OnKeypadWrongPassword() {
    xTaskCreate(wrong_pass_fx, "wpass_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}

void IO_fx::Init() {
    ledcAttachPin(BuzzerPin, BuzzerLEDCChannel);
    ledcWriteTone(BuzzerLEDCChannel, 0);
    ledcWrite(BuzzerLEDCChannel, 0);

    //pinMode(14, OUTPUT);
    //digitalWrite(14, LOW);

    RGB_LED::Init();

    RGB_LED::Set(0, 0, 0);
    RGB_LED::SetR(255, 0, 500, 250);
    RGB_LED::SetG(32, 0, 2000, 250);
    xTaskCreate(status_update_task, "io_task", 4096, NULL, 0, NULL);

    xTaskCreate(startup_fx, "startup_fx", 4096, NULL, 0, NULL);

    pinMode(IndicatorPin, OUTPUT);
}

void IO_fx::UpdateData()
{
    Config::Object["beepWhenOpen"] = beepWhenOpen;
}

void IO_fx::ReadData()
{
    beepWhenOpen = Config::Object["beepWhenOpen"];
}

void IO_fx::Alarm_Blocking() {
    fGUI::ResetScreensaver();
    fGUI::OpenMenu(10);

    //digitalWrite(14, HIGH);
    //delay(1000);
    for (int h = 0; h < 5; h++) {
        for (int j = 0; j < 40; j++) {
            for (int i = 25; i < 40; i++) {
                ledcChangeFrequency(BuzzerLEDCChannel, i * 100, 4);
                ledcWrite(BuzzerLEDCChannel, 8);
                delay(50);
            }
        }
        ledcWriteTone(BuzzerLEDCChannel, 3000);
        delay(10000);
    }
    ledcWrite(BuzzerLEDCChannel, 0);
    //digitalWrite(14, LOW);
}

void IO_fx::Warning() {
    xTaskCreate(warning_fx, "warning_fx", 4096, NULL, 0, NULL);
    fGUI::ResetScreensaver();
}