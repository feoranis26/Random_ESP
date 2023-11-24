#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

String current_pass = "";
String last_pass = "";
bool authorized = false;
long auth_millis = 0;
long wake_millis = 0;
long last_pass_entered = 0;

bool passwordInput = true;
bool wait = true;
long wait_start = 0;
int menu_index = 0;

String current_status;

void setup() {
    lcd.begin(16, 2);
    lcd.clear();

    Serial.begin(9600);
}

bool buttonDown = false;

void select() {
    if (passwordInput)
        sendPassword();
    else if(menu_index == 0)
        sendCmd("lock");
    else if (menu_index == 1)
        sendCmd("unlock");
    else if (menu_index == 2)
        sendCmd("sw_engage");
}

void left() {
    if (passwordInput)
        current_pass += "L";
    else if (menu_index > 0)
        menu_index--;
}
void up() {
    if (passwordInput)
        current_pass += "U";
}
void right() {
    if (passwordInput)
        current_pass += "R";
    else if (menu_index < 2)
        menu_index++;
}

void down() {
    if (passwordInput)
        current_pass += "D";
}

void buttons() {
    int val = analogRead(A0);

    if (val < 1000 && millis() - wake_millis > 20000) {
        Serial.print("wake ;");
        wake_millis = millis();
        return;
    }

    if (val < 1000)
        wake_millis = millis();

    if (!buttonDown) {
        if (abs(val - 600) < 50)
            select();
        else if (abs(val - 400) < 50)
            left();
        else if (abs(val - 100) < 50)
            up();
        else if (abs(val - 0) < 50)
            right();
        else if (abs(val - 250) < 50)
            down();
    }

    buttonDown = val < 1000;
}

void displayCurrentPass() {
    lcd.setCursor(0, 0);
    lcd.print("Enter password:");

    lcd.setCursor(0, 1);
    lcd.print(current_pass);
}

void sendPassword() {
    last_pass = current_pass;
    sendCmd("unlock");

    current_pass = "";
    last_pass_entered = millis();
}

void sendCmd(String cmd) {
    Serial.print(last_pass + " " + cmd + ";");

    last_pass_entered = millis();
}

void passwordInputFx() {
    if (millis() - wake_millis > 20000) {
        lcd.setCursor(0, 0);
        lcd.print("Status:");
        lcd.setCursor(0, 1);
        lcd.print(current_status);

        current_pass = "";
        return;
    }

    if (current_pass.length() > 12)
        current_pass = current_pass.substring(0, 12);

    displayCurrentPass();
}

void menu() { //>LOCK_>UNLK_>E/D
    lcd.setCursor(0, 0);
    lcd.print("Select:");

    lcd.setCursor(1, 1);
    lcd.print("Lock");

    lcd.setCursor(7, 1);
    lcd.print("UnLk");

    lcd.setCursor(13, 1);
    lcd.print("E/D");

    lcd.setCursor(menu_index * 6, 1);
    lcd.print(">");
}

String spliced[32];
void checkSerial() {
    if (Serial.available()) {
        int numSplices = 0;
        String read = "";
        read = Serial.readStringUntil(';');

        int idx = read.indexOf(' ');
        while (idx != -1) {
            idx = read.indexOf(' ');

            String token = read.substring(0, idx);
            //Serial.println(token);
            spliced[numSplices++] = token;
            read = read.substring(idx + 1);
        }

        if(spliced[0] == "status"){
            String status = spliced[1];

            bool authorized_t = true;

            if (status == "auth_err") {
                status = "Wrong password";
                authorized_t = false;
            }
            else if(status == "not_engaged")
                status = "Door not engaged";
            else if (status == "not_locked")
                status = "Door not locked";
            else if (status == "ok")
                status = "OK.";
            else if (status == "cooldown") {
                status = "Please wait.";
                authorized_t = false;

                wait = true;
                wait_start = millis();
            }
            else if (status == "locked") {
                status = "Door locked.";
                authorized_t = false;
            }
            else if (status == "unlocked") {
                status = "Door unlocked.";
                authorized_t = false;
            }
            else if (status == "disengaged") {
                status = "Door disengaged.";
                authorized_t = false;
            }

            if (authorized_t) {
                authorized = true;
                auth_millis = millis();
                passwordInput = false;
                current_status = status;
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Status:");
            lcd.setCursor(0, 1);
            lcd.print(status);

            long startMillis = millis();

            while (analogRead(A0) < 1000)
                delay(10);
            
            while (analogRead(A0) > 1000 && millis() - startMillis < 2500)
                delay(10);
        }
        else if (spliced[0] == "update_status") {
            String status = spliced[1];

            bool authorized_t = true;

            if (status == "locked") {
                status = "Door locked.";
            }
            else if (status == "unlocked") {
                status = "Door unlocked.";
            }
            else if (status == "disengaged") {
                status = "Door disengaged.";
            }

            current_status = status;
        }

        while (Serial.available())
            Serial.read();
    }

}

void wait_alert() {
    lcd.setCursor(0, 0);
    lcd.print("Status:");

    if (millis() % 1500 < 750) {
        lcd.setCursor(0, 1);
        lcd.print("!Please wait.  !");
    }
    else {
        lcd.setCursor(0, 1);
        lcd.print("!  Please wait.!");
    }


    if (millis() - wait_start > 10000)
        wait = false;
}

void loop() {
    lcd.clear();
    buttons();

    if (passwordInput && !wait)
        passwordInputFx();
    else if (!wait)
        menu();
    else
        wait_alert();

    if (millis() - auth_millis > 20000 && !wait) {
        authorized = false;
        passwordInput = true;
    }

    if(last_pass != "" && millis() - last_pass_entered > 20000)
        last_pass = "";

    checkSerial();

    delay(50);
}
