#pragma once

#include <memory>
#include <vector>
#include <wx/app.h>
#include <wx/snglinst.h>
#include <wx/timer.h>

#include "eyerelax/BreakScheduler.hpp"
#include "eyerelax/Localization.hpp"
#include "eyerelax/Settings.hpp"
#include "eyerelax/TrayIcon.hpp"

class BreakWindow;
class SettingsFrame;
class UpcomingBreakWindow;

class EyesRelaxApp final : public wxApp {
public:
    ~EyesRelaxApp() override;
    bool OnInit() override;
    int OnExit() override;

private:
    void Reconfigure();
    void ShowSettings();
    void TogglePause();
    void StartBreak(BreakScheduler::BreakKind kind);
    void CloseBreaks(BreakScheduler::BreakKind kind);
    void ShowUpcomingBreak();
    void DismissUpcomingBreak();
    void CheckShutdownSchedule();
    void UpdateTrayStatus();
    void OnTimer(wxTimerEvent& event);

    Settings settings_;
    Localization localization_;
    std::unique_ptr<wxSingleInstanceChecker> instanceChecker_;
    std::unique_ptr<BreakScheduler> scheduler_;
    std::unique_ptr<TrayIcon> tray_;
    SettingsFrame* settingsFrame_{nullptr};
    UpcomingBreakWindow* upcomingBreakWindow_{nullptr};
    std::vector<BreakWindow*> breakWindows_;
    wxTimer timer_;
    bool breakActive_{false};
    bool upcomingBreakShown_{false};
    bool idleSuspended_{false};
    bool shutdownRequested_{false};
    int shutdownRetrySeconds_{0};
    BreakScheduler::BreakKind activeBreakKind_{BreakScheduler::BreakKind::Short};
};
