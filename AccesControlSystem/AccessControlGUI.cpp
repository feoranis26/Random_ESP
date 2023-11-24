#include "AccessControlGUI.h"

UncenteredTextElement* CardScannedMenu::cardID;
uint32_t CardScannedMenu::bg_col;
String CardScannedMenu::nuidString;
String lastStatusCode = "";

class Lock;
extern int rssi_limit;

void initGUI() {
    ESP32Encoder* e = new ESP32Encoder();
    e->useInternalWeakPullResistors = UP;
    e->attachSingleEdge(EncoderAPin, EncoderBPin);

    fGUI::AddMenu(new DoorControlMenu());   //0
    fGUI::AddMenu(new EngagedMenu());       //1
    fGUI::AddMenu(new DisengagedMenu());    //2
    fGUI::AddMenu(new OpenMenu());          //3
    fGUI::AddMenu(new ClosedMenu());        //4
    fGUI::AddMenu(new LockedMenu());        //5
    fGUI::AddMenu(new UnlockedMenu());      //6
    fGUI::AddMenu(new CardScannedMenu());   //7
    fGUI::AddMenu(new IPMenu());            //8
    fGUI::AddMenu(new EngageWarning());     //9
    fGUI::AddMenu(new ErrorMenu());         //10

    fGUI::AddMenu(new IDMenu());            //11
    fGUI::AddMenu(new IDActionsMenu());     //12
    fGUI::AddMenu(new SettingsMenu());      //13

    fGUI::AddMenu(new AutoEngageMenu());    //14
    fGUI::AddMenu(new EditConfigMenu());    //15


    Serial.println("ok add");

    fGUI::Init(e, EncoderSwPin);
    fGUI::OpenMenu(14);
    Serial.println("ok init");
}

void DoorControlMenu::InitializeElements() {
    AddElement(new Button("Unlock", 64, 30, 64, 16, 0, 1, TFT_BLACK, TFT_WHITE, []() {lastStatusCode = Lock::UnlockDoor(); }, ""));
    AddElement(new Button("Engage", 64, 50, 64, 16, 0, 1, TFT_BLACK, TFT_WHITE, []() {
        if (Lock::isEngaged)
            lastStatusCode = Lock::DisengageDoor();
        else
            fGUI::OpenMenu(9);
        }, ""));

    AddElement(new Button("Config", 64, 90, 64, 16, 0, 1, TFT_BLACK, TFT_ORANGE, []() { fGUI::OpenMenu(13); }, ""));

    t = new UncenteredTextElement("State: ", 8, 106, 0, 1, TFT_WHITE);
    AddElement(t);
}

void DoorControlMenu::Draw() {
    ElementMenu::Draw();

    String text = "";

    if (Lock::isLocked && Lock::isEngaged)
        text += "Locked";
    else if (!Lock::isEngaged)
        text += "Disengaged";
    else
        text += "Unlocked";

    if (!Lock::canChangeState())
        text += ", busy";

    t->t = text + ", " + lastStatusCode;
}

void UnlockedMenu::InitializeElements() {
    AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
    AddElement(new TextElement("Unlocked", 64, 80, 4, 1, TFT_BLACK));
    time = new TextElement("Time to close: 00.0s", 64, 100, 0, 1, TFT_BLACK);

    AddElement(time);
    bg_color = TFT_GREEN;
}

void UnlockedMenu::Draw() {
    ElementMenu::Draw();

    if (!Lock::isOpen)
        time->t = "Time to close: " + String(Lock::GetTimeToClose(), 1) + "s";
    else
        time->t = "Door is open.";

    bg_color = Lock::GetTimeToClose() < 2.0 ? TFT_ORANGE : TFT_GREEN;

    if ((Lock::isLocked || !Lock::isEngaged || Lock::isLocking) && !Lock::isUnlocking)
        fGUI::ExitMenu();
}

void UnlockedMenu::Exit() {
    ElementMenu::Exit();

    fGUI::ExitMenu();
}

IDMenuElement* IDMenu::selected;

void IDMenu::Refresh() {
    for (int i = numElements - 1; i >= 0; i--)
        RemoveElement(i);

    DynamicJsonDocument keys_cnf = Authentication::GetConfig();
    for (String s : keys_cnf["keys"].as<JsonArray>())
        AddElement(new IDMenuElement(s));
}

void IDMenu::Remove() {
    Authentication::RemoveAuth(selected->m_id);
}

void EngageWarning::InitializeElements() {
    AddElement(new TextElement("Authorize to engage.", 64, 64, 1, 1, TFT_BLACK));

    bg_color = TFT_RED;
    timeToClose = 5000;
}

void EngageWarning::Draw() {
    TimedMenu::Draw();

    if (millis() - Authentication::LastAuthenticatedMillis < 20000) {
        Lock::EngageDoor();
        fGUI::ExitMenu();
    }
}

void CardScannedMenu::InitializeElements() {
    AddElement(new TextElement("Card", 64, 32, 4, 1, TFT_BLACK));
    AddElement(new TextElement("Scanned", 64, 80, 4, 1, TFT_BLACK));
    cardID = new UncenteredTextElement("ID: 00000000", 16, 100, 0, 1, TFT_BLACK);

    AddElement(cardID);
    bg_color = TFT_LIGHTGREY;
}

void CardScannedMenu::Draw() {
    TimedMenu::Draw();

    if (bg_col == TFT_RED && millis() - Authentication::LastAuthenticatedMillis < 5000) {
        Authentication::AddAuth(nuidString);

        cardID->t = nuidString + " Added.";
        bg_color = TFT_GREEN;
    }
}

void CardScannedMenu::Enter() {
    TimedMenu::Enter();

    bg_color = bg_col;
    if (bg_col == TFT_GREEN)
        timeToClose = 500;
}


void AutoEngageMenu::InitializeElements() {
    AddElement(new TextElement("Auto", 64, 32, 4, 1, TFT_BLACK));
    AddElement(new TextElement("Engage", 64, 80, 4, 1, TFT_BLACK));
    time = new TextElement("Time to engage: 00.0s", 64, 100, 0, 1, TFT_BLACK);

    AddElement(time);
    bg_color = TFT_ORANGE;
}

void AutoEngageMenu::Draw() {
    ElementMenu::Draw();

    //if (millis() < 10000)
      //  time->t = "Time to engage: " + String((double)(10000 - millis()) / 1000, 1) + "s";
    //else
        time->t = "Enter pass to access.";

    if (!Lock::isEngaged &&/* millis() > 10000 &&*/ Lock::canChangeState())
        Lock::EngageDoor();

    if (millis() - Authentication::LastAuthenticatedMillis < 5000)
        fGUI::ExitMenu();
}

void AutoEngageMenu::Exit() {
    ElementMenu::Exit();

    fGUI::ExitMenu();
}

void EditConfigMenu::InitializeElements() {
    AddElement(new Button("Slnd w/ open: n", 64, 32, 96, 12, 0, 1, TFT_BLACK, TFT_WHITE, []() { Lock::engageSolenoidWhenOpen = !Lock::engageSolenoidWhenOpen; }, ""));
    AddElement(new Button("Beep w/ open: n", 64, 48, 96, 12, 0, 1, TFT_BLACK, TFT_WHITE, []() { IO_fx::beepWhenOpen = !IO_fx::beepWhenOpen; }, ""));
    AddElement(new FloatInputElement("Lock time   ", 16, 64, 0, 1, TFT_BLACK, TFT_WHITE));
    AddElement(new NumberInputElement("Beacon RSSI ", 16, 80, 0, 1, TFT_BLACK, TFT_WHITE));
    AddElement(new Button("Ext", 64, 112, 60, 15, 2, 1, TFT_WHITE, TFT_ORANGE, []() { fGUI::ExitMenu(); fGUI::ExitMenu(); }, ""));
}

void EditConfigMenu::Enter() {
    ElementMenu::Enter();

    ((FloatInputElement*)Elements[2])->Value = Lock::doorLockTimeAfterClose;
    ((NumberInputElement*)Elements[3])->Value = rssi_limit;
}

void EditConfigMenu::Draw() {
    ElementMenu::Draw();

    ((Button*)Elements[0])->t = "Slnd w / open:" + String(Lock::engageSolenoidWhenOpen ? "y" : "n");
    ((Button*)Elements[1])->t = "Beep w / open:" + String(IO_fx::beepWhenOpen ? "y" : "n");
    Lock::doorLockTimeAfterClose = ((FloatInputElement*)Elements[2])->Value;
    rssi_limit = ((NumberInputElement*)Elements[3])->Value;
}

void EditConfigMenu::Exit() {
    ElementMenu::Exit();

    Config::Save();
}