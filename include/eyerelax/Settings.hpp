#pragma once

#include <wx/string.h>

struct Settings {
    int shortWorkMinutes{20};
    int shortBreakSeconds{20};
    int shortBreaksBeforeLong{3};
    int longBreakSeconds{300};
    wxString locale{"system"};
    bool launchAtLogin{false};
    bool allowBreakCancel{true};

    static Settings Load();
    bool Save() const;
};
