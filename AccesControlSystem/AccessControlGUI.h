#ifndef AccessControlGUI_h
#define AccessControlGUI_h

#include <ArduinoJson.h>
#include <WiFi.h>
#include "fGUI.h"
#include "ElementMenu.h"

#include "IO.h"
#include "Lock.h"
#include "Authentication.h"

extern String lastStatusCode;

class DoorControlMenu : public ElementMenu {
    void InitializeElements() override;
    void Draw() override;

    UncenteredTextElement* t;
};

class TimedMenu : public ElementMenu {
protected:
    long timeToClose = 5000;

    void Enter() override {
        ElementMenu::Enter();

        open_millis = millis();
    }

    void Draw() override {
        ElementMenu::Draw();

        if (millis() - open_millis > timeToClose)
            fGUI::ExitMenu();
    }

    void OnEncoderTickBackward() override {
        fGUI::ExitMenu();
    }

    void OnEncoderTickForward() override {
        fGUI::ExitMenu();
    }

    void OnButtonClicked(int id) override {
        fGUI::ExitMenu();
    }
private:
    long open_millis = 0;
};

class EngagedMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Engaged", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_GREEN;
        timeToClose = 1000;
    }
};

class DisengagedMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Disengaged", 64, 80, 2, 1, TFT_BLACK));

        bg_color = TFT_ORANGE;
        timeToClose = 1000;
    }
};

class OpenMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Open", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_ORANGE;
        timeToClose = 1000;
    }
};

class ClosedMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Closed", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_ORANGE;
        timeToClose = 1000;
    }
};

class LockedMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Door", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Locked", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_RED;
        timeToClose = 1500;
    }
};

class UnlockedMenu : public ElementMenu {
    void InitializeElements() override;
    void Draw() override;
    void Exit() override;

    TextElement* time;
};

class EngageWarning : public TimedMenu {
    void InitializeElements() override;
    void Draw() override;
};

class CardScannedMenu : public TimedMenu {
    void InitializeElements() override;
    void Draw() override;
    void Enter() override;

public:
    static UncenteredTextElement* cardID;
    static uint32_t bg_col;
    static String nuidString;
};

class SettingsMenu : public ElementMenu {
    void InitializeElements() override {
        AddElement(new Button("Edit Keys", 64, 32, 64, 16, 0, 1, TFT_BLACK, TFT_WHITE, []() { fGUI::OpenMenu(11); }, ""));
        AddElement(new Button("Edit Conf", 64, 56, 64, 16, 0, 1, TFT_BLACK, TFT_WHITE, []() { fGUI::OpenMenu(15); }, ""));
        AddElement(new Button("Exit", 64, 144, 60, 15, 2, 1, TFT_WHITE, TFT_ORANGE, []() { fGUI::ExitMenu(); }, ""));
    }
};

class IPMenu : public TimedMenu {
    void InitializeElements() override {
        AddElement(new TextElement("WiFi", 64, 32, 4, 1, TFT_BLACK));
        AddElement(new TextElement("Connected", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_LIGHTGREY;
        timeToClose = 1000;
    }

    void Enter() override {
        TimedMenu::Enter();

        AddElement(new TextElement("IP: " + WiFi.localIP().toString(), 64, 100, 0, 1, TFT_BLACK));
    }
};

class IDMenuElement : public ListTextElement {
    void OnClick() override {
        fGUI::OpenMenu(12);
    }

public:
    IDMenuElement(String id) : ListTextElement(id, 1, 1, TFT_WHITE, TFT_BLACK) { m_id = id; }
    String m_id;
};

class IDMenu : public ListMenu {
public:
    static IDMenuElement* selected;
    void Refresh();

    void InitializeElements() override {
        start_x = 24;

        Refresh();
    }

    void Enter() override {
        Refresh();

        ListMenu::Enter();
    }

    void Draw() override {
        ListMenu::Draw();

        selected = (IDMenuElement*)elements[highlightedElementIndex];
    }

    static void Remove();
};

class IDActionsMenu : public ElementMenu {
    void InitializeElements() override {
        AddElement(new Button("Remove", 64, 48, 60, 15, 2, 1, TFT_WHITE, TFT_RED, []() { IDMenu::Remove(); fGUI::ExitMenu(); }, ""));
        AddElement(new Button("Back", 64, 80, 60, 15, 2, 1, TFT_WHITE, TFT_LIGHTGREY, []() { fGUI::ExitMenu(); }, ""));
        AddElement(new Button("Ext List", 64, 112, 60, 15, 2, 1, TFT_WHITE, TFT_ORANGE, []() { fGUI::ExitMenu(); fGUI::ExitMenu(); }, ""));
    }
};

class ErrorMenu : public ElementMenu {
    void InitializeElements() override {
        AddElement(new TextElement("Error", 64, 80, 4, 1, TFT_BLACK));

        bg_color = TFT_RED;
    }
};

class AutoEngageMenu : public ElementMenu {
    void InitializeElements() override;
    void Draw() override;
    void Exit() override;

    TextElement* time;
};

class EditConfigMenu : public ElementMenu {
    void InitializeElements() override;
    void Enter() override;
    void Draw() override;
    void Exit() override;
};

void initGUI();

#endif