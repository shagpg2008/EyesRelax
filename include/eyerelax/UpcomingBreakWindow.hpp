#pragma once

#include <wx/frame.h>
#include <wx/timer.h>

class Localization;

class UpcomingBreakWindow final : public wxFrame {
public:
    explicit UpcomingBreakWindow(const Localization& localization);

private:
    void OnAnimationTick(wxTimerEvent& event);

    wxTimer animationTimer_;
    wxPoint startPosition_;
    wxPoint targetPosition_;
    long elapsedMilliseconds_{0};
};
