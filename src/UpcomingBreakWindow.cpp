#include "eyerelax/UpcomingBreakWindow.hpp"

#include "eyerelax/Icons.hpp"
#include "eyerelax/Localization.hpp"
#ifdef __WXOSX__
#include "eyerelax/MacWindowActivation.hpp"
#endif
#include <algorithm>
#include <wx/artprov.h>
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
    : wxFrame(nullptr, wxID_ANY, localization.Text("app.name"),
              wxDefaultPosition, wxDefaultSize,
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxCAPTION | wxSYSTEM_MENU |
                  wxCLOSE_BOX | wxBORDER_SIMPLE),
      animationTimer_(this) {
    SetIcon(CreateEyeIcon(true));
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxHORIZONTAL);
    root->SetMinSize(FromDIP(wxSize(332, 64)));
    auto* icon = new wxStaticBitmap(
        panel, wxID_ANY,
        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX,
                                 FromDIP(wxSize(40, 40))));
    auto* title = new wxStaticText(panel, wxID_ANY, localization.Text("app.name"));
    auto* text = new wxStaticText(panel, wxID_ANY, localization.Text("break.upcoming"));
    wxFont font = text->GetFont();
    font.SetPointSize(font.GetPointSize() + 1);
    text->SetFont(font);
    title->SetFont(font);
    auto* message = new wxBoxSizer(wxVERTICAL);
    message->Add(title, 0, wxBOTTOM, FromDIP(4));
    message->Add(text, 0);
    root->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(16));
    root->Add(message, 1, wxALIGN_CENTER_VERTICAL);
    panel->SetSizer(root);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(panel, 1, wxEXPAND | wxALL, FromDIP(16));
    // Size to the two text rows instead of enforcing a fixed aspect ratio.
    SetSizerAndFit(outer);

    int displayIndex = wxDisplay::GetFromPoint(wxGetMousePosition());
    if (displayIndex == wxNOT_FOUND) displayIndex = 0;
    const wxRect workArea = wxDisplay(static_cast<unsigned>(displayIndex)).GetClientArea();
    const wxSize size = GetSize();
    targetPosition_ = wxPoint(workArea.GetRight() - size.x - kRightMargin + 1,
                              workArea.GetBottom() - size.y - kBottomMargin + 1);
#ifdef __WXOSX__
    // GetClientArea() uses NSScreen.visibleFrame, excluding the Dock and menu
    // bar. Start with the entire reminder above that boundary, rather than
    // placing its top edge below it and hiding the text behind the Dock.
    targetPosition_.x = std::max(workArea.GetLeft() + FromDIP(kRightMargin),
                                 targetPosition_.x);
    targetPosition_.y = std::max(workArea.GetTop() + FromDIP(kBottomMargin),
                                 targetPosition_.y);
    startPosition_ = targetPosition_;
    targetPosition_.y = std::max(workArea.GetTop() + FromDIP(kBottomMargin),
                                 startPosition_.y - FromDIP(24));
#else
    startPosition_ = wxPoint(targetPosition_.x, workArea.GetBottom() + 1);
#endif
    Move(startPosition_);

    Bind(wxEVT_TIMER, &UpcomingBreakWindow::OnAnimationTick, this);
#ifdef __WXOSX__
    ShowMacReminderWithoutActivating(*this);
#else
    ShowWithoutActivating();
#endif
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
