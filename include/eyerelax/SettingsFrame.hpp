#pragma once

#include <functional>
#include <wx/frame.h>
#include <wx/timer.h>

class Localization;
struct Settings;
class wxChoice;
class wxCheckBox;
class wxSpinCtrl;
class wxStaticText;

class SettingsFrame final : public wxFrame {
public:
    SettingsFrame(Settings& settings, const Localization& localization,
                  std::function<void()> onSaved,
                  std::function<long long()> secondsUntilBreak);

private:
    void UpdateNextBreak();

    Settings& settings_;
    const Localization& localization_;
    std::function<void()> onSaved_;
    std::function<long long()> secondsUntilBreak_;
    wxSpinCtrl* shortWork_{nullptr};
    wxSpinCtrl* shortBreak_{nullptr};
    wxSpinCtrl* shortBreaksBeforeLong_{nullptr};
    wxSpinCtrl* longBreak_{nullptr};
    wxChoice* locale_{nullptr};
    wxCheckBox* launchAtLogin_{nullptr};
    wxCheckBox* allowBreakCancel_{nullptr};
    wxStaticText* nextBreak_{nullptr};
    wxTimer refreshTimer_;
};
