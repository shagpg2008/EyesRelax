#include "eyerelax/UpcomingBreakWindow.hpp"

#include "eyerelax/Icons.hpp"
#include "eyerelax/Localization.hpp"
#include <algorithm>
#include <wx/display.h>
#include <wx/mousestate.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

namespace {
constexpr int kAnimationIntervalMs = 20;
constexpr int kRiseDurationMs = 900;
constexpr int kVisibleDurationMs = 5000;
constexpr int kRightMargin = 12;
constexpr int kBottomMargin = 12;
}

UpcomingBreakWindow::UpcomingBreakWindow(const Localization& localization)
    : wxFrame(nullptr, wxID_ANY, localization.Text("break.upcomingTitle"),
              wxDefaultPosition, wxSize(350, 112),
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxCAPTION | wxBORDER_SIMPLE),
      animationTimer_(this) {
    SetIcon(CreateEyeIcon(true));
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxHORIZONTAL);
    auto* icon = new wxStaticBitmap(panel, wxID_ANY, wxBitmap(CreateEyeIcon(true)));
    auto* text = new wxStaticText(panel, wxID_ANY, localization.Text("break.upcoming"));
    wxFont font = text->GetFont();
    font.SetPointSize(font.GetPointSize() + 1);
    text->SetFont(font);
    text->Wrap(260);
    root->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    root->Add(text, 1, wxALIGN_CENTER_VERTICAL);
    panel->SetSizer(root);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(panel, 1, wxEXPAND | wxALL, 14);
    SetSizer(outer);

    int displayIndex = wxDisplay::GetFromPoint(wxGetMousePosition());
    if (displayIndex == wxNOT_FOUND) displayIndex = 0;
    const wxRect workArea = wxDisplay(static_cast<unsigned>(displayIndex)).GetClientArea();
    const wxSize size = GetSize();
    targetPosition_ = wxPoint(workArea.GetRight() - size.x - kRightMargin + 1,
                              workArea.GetBottom() - size.y - kBottomMargin + 1);
    startPosition_ = wxPoint(targetPosition_.x, workArea.GetBottom() + 1);
    Move(startPosition_);

    Bind(wxEVT_TIMER, &UpcomingBreakWindow::OnAnimationTick, this);
    ShowWithoutActivating();
    animationTimer_.Start(kAnimationIntervalMs);
}

void UpcomingBreakWindow::OnAnimationTick(wxTimerEvent&) {
    elapsedMilliseconds_ += kAnimationIntervalMs;
    if (elapsedMilliseconds_ >= kVisibleDurationMs) {
        animationTimer_.Stop();
        Destroy();
        return;
    }

    const double progress = std::min(1.0, static_cast<double>(elapsedMilliseconds_) /
                                              kRiseDurationMs);
    const double eased = 1.0 - (1.0 - progress) * (1.0 - progress);
    const int y = startPosition_.y +
                  static_cast<int>((targetPosition_.y - startPosition_.y) * eased);
    Move(targetPosition_.x, y);
}
