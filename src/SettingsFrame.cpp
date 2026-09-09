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
              wxSize(470, 440)), settings_(settings), localization_(t),
      onSaved_(std::move(onSaved)), secondsUntilBreak_(std::move(secondsUntilBreak)),
      refreshTimer_(this) {
    SetIcon(CreateEyeIcon(true));
    auto* panel = new wxPanel(this);
    auto* form = new wxFlexGridSizer(2, 12, 12);
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
    form->Add(new wxStaticText(panel, wxID_ANY, t.Text("settings.nextBreak")), 0,
              wxALIGN_CENTER_VERTICAL);
    nextBreak_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    form->Add(nextBreak_, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    auto* save = new wxButton(panel, wxID_SAVE, t.Text("settings.save"));
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(form, 1, wxEXPAND | wxALL, 24);
    root->Add(save, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 24);
    panel->SetSizer(root);
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
