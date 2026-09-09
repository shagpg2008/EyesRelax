#pragma once

#include <functional>
#include <vector>
#include <wx/frame.h>
#include <wx/timer.h>

class Localization;
class wxGauge;
class wxStaticText;

class BreakWindow final : public wxFrame {
public:
    BreakWindow(const wxRect& geometry, int seconds, bool allowCancel,
                const Localization& localization,
                std::function<void()> onFinished);

private:
    void OnTick(wxTimerEvent& event);
    void Finish();
    void UpdateRemaining();

    int totalSeconds_;
    int remainingSeconds_;
    const Localization& localization_;
    std::function<void()> onFinished_;
    wxStaticText* remainingLabel_{nullptr};
    wxGauge* progress_{nullptr};
    wxTimer timer_;
    bool finished_{false};
};
