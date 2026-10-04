#include "eyerelax/App.hpp"

#include "eyerelax/BreakWindow.hpp"
#include "eyerelax/IdleTime.hpp"
#include "eyerelax/PowerControl.hpp"
#include "eyerelax/SettingsFrame.hpp"
#include "eyerelax/ShutdownSchedule.hpp"
#include "eyerelax/TrayIcon.hpp"
#include "eyerelax/UpcomingBreakWindow.hpp"
#include <wx/display.h>
#include <wx/imagpng.h>
#include <wx/datetime.h>
#include <wx/snglinst.h>

namespace {
constexpr auto kIdleResetThreshold = std::chrono::minutes(3);
}

EyesRelaxApp::~EyesRelaxApp() = default;

bool EyesRelaxApp::OnInit() {
    if (!wxApp::OnInit()) return false;
    SetExitOnFrameDelete(false);
    wxImage::AddHandler(new wxPNGHandler);
    SetAppName("EyesRelax");
    instanceChecker_ = std::make_unique<wxSingleInstanceChecker>();
    if (instanceChecker_->IsAnotherRunning()) return false;
    settings_ = Settings::Load();
    localization_.Load(settings_.locale);
    scheduler_ = std::make_unique<BreakScheduler>(settings_.shortWorkMinutes,
                                                   settings_.shortBreaksBeforeLong);
    tray_ = std::make_unique<TrayIcon>(
        localization_, [this] { ShowSettings(); }, [this] { TogglePause(); },
        [this] { StartBreak(BreakScheduler::BreakKind::Short); },
        [this] { ExitMainLoop(); }, [this] { return scheduler_->IsPaused(); });
    timer_.SetOwner(this);
    Bind(wxEVT_TIMER, &EyesRelaxApp::OnTimer, this);
    timer_.Start(1000);
    UpdateTrayStatus();
#ifdef __WXGTK__
    // GNOME and some other Linux desktops hide legacy tray icons. Always show
    // a visible window on launch so starting the application has clear feedback.
    CallAfter([this] { ShowSettings(); });
#endif
    return true;
}

int EyesRelaxApp::OnExit() {
    timer_.Stop();
    DismissUpcomingBreak();
    breakWindows_.clear();
    tray_.reset();
    instanceChecker_.reset();
    return wxApp::OnExit();
}

void EyesRelaxApp::Reconfigure() {
    const bool wasPaused = scheduler_->IsPaused();
    localization_.Load(settings_.locale);
    scheduler_->Configure(settings_.shortWorkMinutes, settings_.shortBreaksBeforeLong);
    upcomingBreakShown_ = false;
    shutdownRequested_ = false;
    shutdownRetrySeconds_ = 0;
    DismissUpcomingBreak();
    if (wasPaused) scheduler_->Pause(true);
    tray_.reset();
    tray_ = std::make_unique<TrayIcon>(
        localization_, [this] { ShowSettings(); }, [this] { TogglePause(); },
        [this] { StartBreak(BreakScheduler::BreakKind::Short); },
        [this] { ExitMainLoop(); }, [this] { return scheduler_->IsPaused(); });
    UpdateTrayStatus();
}

void EyesRelaxApp::ShowSettings() {
    if (settingsFrame_) {
        settingsFrame_->Raise();
        return;
    }
    settingsFrame_ = new SettingsFrame(
        settings_, localization_, [this] { Reconfigure(); }, [this] {
            if (scheduler_->IsPaused() || idleSuspended_) return -1LL;
            if (breakActive_) return 0LL;
            return static_cast<long long>(scheduler_->Remaining().count());
        });
    settingsFrame_->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& event) {
        settingsFrame_ = nullptr;
        event.Skip();
    });
    settingsFrame_->Show();
}

void EyesRelaxApp::TogglePause() {
    const bool pause = !scheduler_->IsPaused();
    scheduler_->Pause(pause);
    if (pause) DismissUpcomingBreak();
    if (pause && breakActive_) CloseBreaks(activeBreakKind_);
    UpdateTrayStatus();
}

void EyesRelaxApp::StartBreak(BreakScheduler::BreakKind kind) {
    if (breakActive_) return;
    DismissUpcomingBreak();
    breakActive_ = true;
    activeBreakKind_ = kind;
    UpdateTrayStatus();
    const int seconds = kind == BreakScheduler::BreakKind::Long
                            ? settings_.longBreakSeconds : settings_.shortBreakSeconds;
    const unsigned displays = wxDisplay::GetCount();
    for (unsigned i = 0; i < displays; ++i) {
        const wxRect geometry = wxDisplay(i).GetGeometry();
        breakWindows_.push_back(new BreakWindow(
            geometry, seconds, settings_.allowBreakCancel, localization_,
            [this, kind] { CloseBreaks(kind); }));
    }
}

void EyesRelaxApp::CloseBreaks(BreakScheduler::BreakKind kind) {
    if (!breakActive_) return;
    breakActive_ = false;
    for (auto* window : breakWindows_) window->Destroy();
    breakWindows_.clear();
    scheduler_->Complete(kind);
    upcomingBreakShown_ = false;
    UpdateTrayStatus();
}

void EyesRelaxApp::ShowUpcomingBreak() {
    if (upcomingBreakWindow_) return;
    upcomingBreakWindow_ = new UpcomingBreakWindow(localization_);
    upcomingBreakWindow_->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& event) {
        if (event.GetEventObject() == upcomingBreakWindow_) upcomingBreakWindow_ = nullptr;
        event.Skip();
    });
}

void EyesRelaxApp::DismissUpcomingBreak() {
    if (!upcomingBreakWindow_) return;
    UpcomingBreakWindow* window = upcomingBreakWindow_;
    upcomingBreakWindow_ = nullptr;
    window->Destroy();
}

void EyesRelaxApp::CheckShutdownSchedule() {
    if (!settings_.shutdownScheduleEnabled) {
        shutdownRequested_ = false;
        shutdownRetrySeconds_ = 0;
        return;
    }
    const wxDateTime now = wxDateTime::Now();
    const int secondsSinceMidnight = now.GetHour() * 3600 + now.GetMinute() * 60 +
                                     now.GetSecond();
    const bool inside = IsWithinShutdownWindow(secondsSinceMidnight,
                                                settings_.shutdownStartSeconds,
                                                settings_.shutdownEndSeconds);
    if (!inside) {
        shutdownRequested_ = false;
        shutdownRetrySeconds_ = 0;
        return;
    }
    if (shutdownRequested_) return;
    if (shutdownRetrySeconds_ > 0) {
        --shutdownRetrySeconds_;
        return;
    }
    if (RequestSystemShutdown()) {
        shutdownRequested_ = true;
    } else {
        shutdownRetrySeconds_ = 60;
    }
}

void EyesRelaxApp::UpdateTrayStatus() {
    if (!tray_) return;
    wxString status;
    if (scheduler_->IsPaused()) {
        status = localization_.Text("tray.paused");
    } else if (breakActive_) {
        status = localization_.Text("settings.restingNow");
    } else if (idleSuspended_) {
        status = localization_.Text("tray.idle");
    } else {
        const long long remaining = scheduler_->Remaining().count();
        status = localization_.Text("settings.nextBreak") + " " +
                 wxString::Format("%02lld:%02lld:%02lld", remaining / 3600,
                                  (remaining / 60) % 60, remaining % 60);
    }
    tray_->SetStatus(status);
}

void EyesRelaxApp::OnTimer(wxTimerEvent&) {
    UpdateTrayStatus();
    if (scheduler_->IsPaused()) return;
    CheckShutdownSchedule();
    if (breakActive_) return;
    const auto idleTime = GetSystemIdleTime();
    if (idleTime && *idleTime >= kIdleResetThreshold) {
        if (!idleSuspended_) {
            idleSuspended_ = true;
            scheduler_->Reset();
            upcomingBreakShown_ = false;
            DismissUpcomingBreak();
            UpdateTrayStatus();
        }
        return;
    }
    if (idleSuspended_) {
        idleSuspended_ = false;
        scheduler_->Reset();
        upcomingBreakShown_ = false;
        UpdateTrayStatus();
    }
    const long long remaining = scheduler_->Remaining().count();
    if (!upcomingBreakShown_ && remaining > 0 && remaining <= 10) {
        upcomingBreakShown_ = true;
        ShowUpcomingBreak();
    }
    if (scheduler_->IsDue()) StartBreak(scheduler_->DueKind());
}
