#include "eyerelax/SettingsFrame.hpp"

#include "eyerelax/Icons.hpp"
#include "eyerelax/Localization.hpp"
#include "eyerelax/Settings.hpp"
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

SettingsFrame::SettingsFrame(Settings& settings, const Localization& t,
                             std::function<void()> onSaved,
                             std::function<long long()> secondsUntilBreak)
    : wxFrame(nullptr, wxID_ANY, t.Text("settings.title"), wxDefaultPosition,
              wxDefaultSize), settings_(settings), localization_(t),
      onSaved_(std::move(onSaved)), secondsUntilBreak_(std::move(secondsUntilBreak)),
      refreshTimer_(this) {
    SetIcon(CreateEyeIcon(true));
    auto* panel = new wxPanel(this);
    auto* form = new wxFlexGridSizer(2, FromDIP(12), FromDIP(12));
    form->AddGrowableCol(1, 1);
    auto addSpin = [&](const wxString& label, int value, int min, int max, wxSpinCtrl*& control) {
        form->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        control = new wxSpinCtrl(panel, wxID_ANY);
        control->SetRange(min, max);
        control->SetValue(value);
        form->Add(control, 1, wxEXPAND);
    };
    addSpin(t.Text("settings.shortWork"), settings.shortWorkMinutes, 1, 240, shortWork_);
    addSpin(t.Text("settings.shortBreak"), settings.shortBreakSeconds, 5, 3600, shortBreak_);
    addSpin(t.Text("settings.longWork"), settings.shortBreaksBeforeLong, 1, 100,
            shortBreaksBeforeLong_);
    addSpin(t.Text("settings.longBreak"), settings.longBreakSeconds, 5, 7200, longBreak_);
    auto addTime = [&](const wxString& label, int totalSeconds, wxSpinCtrl*& hour,
                       wxSpinCtrl*& minute, wxSpinCtrl*& second) {
        form->Add(new wxStaticText(panel, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        auto* time = new wxBoxSizer(wxHORIZONTAL);
        auto addPart = [&](int value, int maximum, wxSpinCtrl*& control) {
            control = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                     FromDIP(wxSize(72, -1)));
            control->SetRange(0, maximum);
            control->SetValue(value);
            time->Add(control, 0, wxALIGN_CENTER_VERTICAL);
        };
        addPart(totalSeconds / 3600, 23, hour);
        time->Add(new wxStaticText(panel, wxID_ANY, ":"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(4));
        addPart((totalSeconds / 60) % 60, 59, minute);
        time->Add(new wxStaticText(panel, wxID_ANY, ":"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(4));
        addPart(totalSeconds % 60, 59, second);
        form->Add(time, 1, wxEXPAND);
    };
    form->Add(new wxStaticText(panel, wxID_ANY, t.Text("settings.language")), 0,
              wxALIGN_CENTER_VERTICAL);
    wxArrayString choices;
    choices.Add(t.Text("settings.systemLanguage"));
    choices.Add(t.Text("language.english"));
    choices.Add(t.Text("language.zhHans"));
    choices.Add(t.Text("language.zhHant"));
    choices.Add(t.Text("language.french"));
    choices.Add(t.Text("language.korean"));
    choices.Add(t.Text("language.japanese"));
    choices.Add(t.Text("language.russian"));
    choices.Add(t.Text("language.german"));
    choices.Add(t.Text("language.bulgarian"));
    choices.Add(t.Text("language.czech"));
    choices.Add(t.Text("language.spanish"));
    choices.Add(t.Text("language.estonian"));
    choices.Add(t.Text("language.croatian"));
    choices.Add(t.Text("language.italian"));
    choices.Add(t.Text("language.polish"));
    choices.Add(t.Text("language.portugueseBrazil"));
    choices.Add(t.Text("language.slovenian"));
    choices.Add(t.Text("language.turkish"));
    choices.Add(t.Text("language.vietnamese"));
    locale_ = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    const wxString codes[] = {"system", "en", "zh-Hans", "zh-Hant", "fr", "ko", "ja",
                              "ru", "de", "bg", "cs", "es", "et", "hr", "it", "pl",
                              "pt-BR", "sl", "tr", "vi"};
    int selected = 0;
    for (int i = 0; i < 20; ++i) if (settings.locale == codes[i]) selected = i;
    locale_->SetSelection(selected);
    form->Add(locale_, 1, wxEXPAND);
    form->AddSpacer(1);
    launchAtLogin_ = new wxCheckBox(panel, wxID_ANY, t.Text("settings.autoStart"));
    launchAtLogin_->SetValue(settings.launchAtLogin);
    form->Add(launchAtLogin_, 1, wxEXPAND);
    form->AddSpacer(1);
    allowBreakCancel_ = new wxCheckBox(panel, wxID_ANY, t.Text("settings.allowCancel"));
    allowBreakCancel_->SetValue(settings.allowBreakCancel);
    form->Add(allowBreakCancel_, 1, wxEXPAND);
    form->AddSpacer(1);
    shutdownScheduleEnabled_ = new wxCheckBox(
        panel, wxID_ANY, t.Text("settings.shutdownEnabled"));
    shutdownScheduleEnabled_->SetValue(settings.shutdownScheduleEnabled);
    form->Add(shutdownScheduleEnabled_, 1, wxEXPAND);
    addTime(t.Text("settings.shutdownStart"), settings.shutdownStartSeconds,
            shutdownStartHour_, shutdownStartMinute_, shutdownStartSecond_);
    addTime(t.Text("settings.shutdownEnd"), settings.shutdownEndSeconds,
            shutdownEndHour_, shutdownEndMinute_, shutdownEndSecond_);
    auto updateShutdownControls = [this] {
        const bool enabled = shutdownScheduleEnabled_->GetValue();
        shutdownStartHour_->Enable(enabled);
        shutdownStartMinute_->Enable(enabled);
        shutdownStartSecond_->Enable(enabled);
        shutdownEndHour_->Enable(enabled);
        shutdownEndMinute_->Enable(enabled);
        shutdownEndSecond_->Enable(enabled);
    };
    shutdownScheduleEnabled_->Bind(wxEVT_CHECKBOX,
        [updateShutdownControls](wxCommandEvent&) { updateShutdownControls(); });
    updateShutdownControls();
    form->Add(new wxStaticText(panel, wxID_ANY, t.Text("settings.nextBreak")), 0,
              wxALIGN_CENTER_VERTICAL);
    nextBreak_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    form->Add(nextBreak_, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    auto* save = new wxButton(panel, wxID_SAVE, t.Text("settings.save"));
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->SetMinSize(FromDIP(wxSize(432, 448)));
    root->Add(form, 1, wxEXPAND | wxALL, FromDIP(24));
    root->Add(save, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));
    panel->SetSizer(root);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(panel, 1, wxEXPAND);
    SetSizerAndFit(outer);
    save->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        static const wxString codes[] = {"system", "en", "zh-Hans", "zh-Hant", "fr", "ko",
                                         "ja", "ru", "de", "bg", "cs", "es", "et", "hr",
                                         "it", "pl", "pt-BR", "sl", "tr", "vi"};
        settings_.shortWorkMinutes = shortWork_->GetValue();
        settings_.shortBreakSeconds = shortBreak_->GetValue();
        settings_.shortBreaksBeforeLong = shortBreaksBeforeLong_->GetValue();
        settings_.longBreakSeconds = longBreak_->GetValue();
        settings_.locale = codes[locale_->GetSelection()];
        settings_.launchAtLogin = launchAtLogin_->GetValue();
        settings_.allowBreakCancel = allowBreakCancel_->GetValue();
        settings_.shutdownScheduleEnabled = shutdownScheduleEnabled_->GetValue();
        settings_.shutdownStartSeconds = shutdownStartHour_->GetValue() * 3600 +
                                         shutdownStartMinute_->GetValue() * 60 +
                                         shutdownStartSecond_->GetValue();
        settings_.shutdownEndSeconds = shutdownEndHour_->GetValue() * 3600 +
                                       shutdownEndMinute_->GetValue() * 60 +
                                       shutdownEndSecond_->GetValue();
        if (settings_.shutdownScheduleEnabled) {
            settings_.launchAtLogin = true;
            launchAtLogin_->SetValue(true);
        }
        settings_.Save();
        if (onSaved_) onSaved_();
        Close();
    });
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { UpdateNextBreak(); });
    UpdateNextBreak();
    refreshTimer_.Start(1000);
    Centre(wxBOTH);
}

void SettingsFrame::UpdateNextBreak() {
    const long long remaining = secondsUntilBreak_ ? secondsUntilBreak_() : -1;
    if (remaining < 0) {
        nextBreak_->SetLabel(localization_.Text("settings.breakDisabled"));
        return;
    }
    if (remaining == 0) {
        nextBreak_->SetLabel(localization_.Text("settings.restingNow"));
        return;
    }
    const long long hours = remaining / 3600;
    const long long minutes = (remaining / 60) % 60;
    const long long seconds = remaining % 60;
    nextBreak_->SetLabel(wxString::Format("%02lld:%02lld:%02lld", hours, minutes, seconds));
}
