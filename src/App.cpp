#include "eyerelax/App.hpp"

#include "eyerelax/BreakWindow.hpp"
#include "eyerelax/IdleTime.hpp"
#include "eyerelax/SettingsFrame.hpp"
#include "eyerelax/TrayIcon.hpp"
#include "eyerelax/UpcomingBreakWindow.hpp"
#include <wx/display.h>
#include <wx/imagpng.h>
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
    DismissUpcomingBreak();
    if (wasPaused) scheduler_->Pause(true);
    tray_.reset();
    tray_ = std::make_unique<TrayIcon>(
        localization_, [this] { ShowSettings(); }, [this] { TogglePause(); },
        [this] { StartBreak(BreakScheduler::BreakKind::Short); },
        [this] { ExitMainLoop(); }, [this] { return scheduler_->IsPaused(); });
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
}

void EyesRelaxApp::StartBreak(BreakScheduler::BreakKind kind) {
    if (breakActive_) return;
    DismissUpcomingBreak();
    breakActive_ = true;
    activeBreakKind_ = kind;
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

void EyesRelaxApp::OnTimer(wxTimerEvent&) {
    if (breakActive_ || scheduler_->IsPaused()) return;
    const auto idleTime = GetSystemIdleTime();
    if (idleTime && *idleTime >= kIdleResetThreshold) {
        if (!idleSuspended_) {
            idleSuspended_ = true;
            scheduler_->Reset();
            upcomingBreakShown_ = false;
            DismissUpcomingBreak();
        }
        return;
    }
    if (idleSuspended_) {
        idleSuspended_ = false;
        scheduler_->Reset();
        upcomingBreakShown_ = false;
    }
    const long long remaining = scheduler_->Remaining().count();
    if (!upcomingBreakShown_ && remaining > 0 && remaining <= 10) {
        upcomingBreakShown_ = true;
        ShowUpcomingBreak();
    }
    if (scheduler_->IsDue()) StartBreak(scheduler_->DueKind());
}
