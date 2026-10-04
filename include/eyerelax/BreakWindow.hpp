#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <wx/frame.h>
#include <wx/timer.h>

class Localization;
class wxPanel;
class wxStaticText;

class BreakWindow final : public wxFrame {
public:
    BreakWindow(const wxRect& geometry, int seconds, bool allowCancel,
                const Localization& localization,
                std::function<void()> onFinished);
    ~BreakWindow() override;

private:
#ifdef __WXOSX__
    void OnAppActivation(wxActivateEvent& event);
#endif
    void OnTick(wxTimerEvent& event);
    void OnFadeTick(wxTimerEvent& event);
    void Finish();
    void UpdateRemaining();

    int totalSeconds_;
    int remainingSeconds_;
    const Localization& localization_;
    std::function<void()> onFinished_;
    wxStaticText* remainingLabel_{nullptr};
    wxPanel* progress_{nullptr};
    wxTimer timer_;
    wxTimer fadeTimer_;
    std::chrono::steady_clock::time_point fadeStarted_;
    bool finished_{false};
};
