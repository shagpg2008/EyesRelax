#include "eyerelax/BreakWindow.hpp"

#include "eyerelax/Localization.hpp"
#ifdef __WXOSX__
#include "eyerelax/MacWindowActivation.hpp"
#include <wx/app.h>
#endif
#include <algorithm>
#include <wx/control.h>
#include <wx/dcbuffer.h>
#include <wx/mousestate.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {
constexpr int kFadeDurationMs = 2000;
constexpr int kFadeIntervalMs = 20;

class CancelButton final : public wxControl {
public:
    CancelButton(wxWindow* parent, const wxString& label)
        : wxControl(parent, wxID_CANCEL, wxDefaultPosition, wxSize(100, 42),
                    wxBORDER_NONE | wxWANTS_CHARS), label_(label) {
        wxFont font = GetFont();
        font.SetPointSize(font.GetPointSize() + 6);
        SetFont(font);
        const wxSize textSize = GetTextExtent(label_);
        SetMinSize(wxSize(std::max(100, kTextAreaLeft + textSize.x + 18),
                          std::max(42, textSize.y + 12)));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        Bind(wxEVT_PAINT, &CancelButton::OnPaint, this);
        Bind(wxEVT_SIZE, [this](wxSizeEvent& event) { Refresh(); event.Skip(); });
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { hovered_ = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { hovered_ = false; Refresh(); });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { pressed_ = true; SetFocus(); Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& event) {
            const bool activate = pressed_ && GetClientRect().Contains(event.GetPosition());
            pressed_ = false;
            Refresh();
            if (activate) Activate();
        });
        Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
            const int key = event.GetKeyCode();
            if (key == WXK_SPACE || key == WXK_RETURN || key == WXK_NUMPAD_ENTER || key == WXK_ESCAPE)
                Activate();
            else
                event.Skip();
        });
        Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) { Refresh(); event.Skip(); });
        Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& event) { pressed_ = false; Refresh(); event.Skip(); });
    }

private:
    static constexpr int kIconCentreX = 18;
    static constexpr int kIconRadius = 13;
    static constexpr int kTextAreaLeft = kIconCentreX + kIconRadius + 1;

    void Activate() {
        wxCommandEvent event(wxEVT_BUTTON, GetId());
        event.SetEventObject(this);
        ProcessWindowEvent(event);
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(wxColour(0, 0, 0)));
        dc.Clear();

        const wxRect bounds = GetClientRect();
        const wxColour border = HasFocus() ? wxColour(0, 120, 215) : wxColour(175, 175, 175);
        const wxColour face = pressed_ ? wxColour(225, 225, 225)
                                      : hovered_ ? wxColour(247, 247, 247)
                                                 : wxColour(255, 255, 255);
        dc.SetPen(wxPen(border, HasFocus() ? 2 : 1));
        dc.SetBrush(wxBrush(face));
        dc.DrawRoundedRectangle(bounds.x + 1, bounds.y + 1,
                                bounds.width - 2, bounds.height - 2, 6);

        const int centreY = bounds.height / 2;
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxColour(241, 72, 38)));
        dc.DrawCircle(kIconCentreX, centreY, kIconRadius);
        dc.SetPen(wxPen(wxColour(255, 255, 255), 2));
        dc.DrawLine(13, centreY - 5, 23, centreY + 5);
        dc.DrawLine(23, centreY - 5, 13, centreY + 5);

        dc.SetFont(GetFont());
        dc.SetTextForeground(wxColour(25, 25, 25));
        dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
        int textWidth = 0, textHeight = 0;
        dc.GetTextExtent(label_, &textWidth, &textHeight);
        // Centre within the blank space between the icon and the inner border.
        const int textAreaWidth = bounds.width - 2 - kTextAreaLeft;
        dc.DrawText(label_, kTextAreaLeft + (textAreaWidth - textWidth) / 2,
                    (bounds.height - textHeight) / 2);
    }

    wxString label_;
    bool hovered_{false};
    bool pressed_{false};
};
} // namespace

BreakWindow::BreakWindow(const wxRect& geometry, int seconds, bool allowCancel,
                         const Localization& localization,
                         std::function<void()> onFinished)
    : wxFrame(nullptr, wxID_ANY, wxEmptyString, geometry.GetPosition(), geometry.GetSize(),
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxBORDER_NONE),
      totalSeconds_(std::max(1, seconds)), remainingSeconds_(std::max(1, seconds)),
      localization_(localization),
      onFinished_(std::move(onFinished)), timer_(this), fadeTimer_(this) {
    const wxColour background(0, 0, 0);
    SetBackgroundColour(background);
    auto* panel = new wxPanel(this);
    panel->SetBackgroundColour(background);
    auto* layout = new wxBoxSizer(wxVERTICAL);
    auto* controls = new wxBoxSizer(wxHORIZONTAL);

    auto* countdownBox = new wxPanel(panel, wxID_ANY, wxDefaultPosition,
                                     wxSize(120, 42), wxBORDER_SIMPLE);
    countdownBox->SetBackgroundColour(wxColour(255, 255, 255));
    remainingLabel_ = new wxStaticText(
        countdownBox, wxID_ANY, "00:00", wxDefaultPosition, wxDefaultSize,
        wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    remainingLabel_->SetBackgroundColour(wxColour(255, 255, 255));
    remainingLabel_->SetForegroundColour(wxColour(80, 80, 80));
    wxFont timeFont = remainingLabel_->GetFont();
    timeFont.SetPointSize(20);
    remainingLabel_->SetFont(timeFont);
    // Native static text does not consistently support vertical alignment.
    // Centre its natural text height inside the fixed-height white box.
    auto* countdownLayout = new wxBoxSizer(wxVERTICAL);
    countdownLayout->AddStretchSpacer();
    countdownLayout->Add(remainingLabel_, 0, wxEXPAND);
    countdownLayout->AddStretchSpacer();
    countdownBox->SetSizer(countdownLayout);

    // Paint both colours ourselves: native GTK gauges can override them with
    // theme colours, making the remaining time indistinguishable from the track.
    progress_ = new wxPanel(panel, wxID_ANY, wxDefaultPosition,
                            wxSize(-1, 42), wxBORDER_NONE);
    progress_->SetBackgroundStyle(wxBG_STYLE_PAINT);
    progress_->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(progress_);
        dc.SetBackground(wxBrush(wxColour(225, 225, 225)));
        dc.Clear();
        const wxSize size = progress_->GetClientSize();
        const int remaining = std::clamp(remainingSeconds_, 0, totalSeconds_);
        const int width = static_cast<int>(
            static_cast<long long>(size.x) * remaining / totalSeconds_);
        if (width > 0 && size.y > 0) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(wxColour(0, 128, 0)));
            dc.DrawRectangle(0, 0, width, size.y);
        }
    });
    progress_->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        progress_->Refresh();
        event.Skip();
    });

    controls->Add(countdownBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    controls->Add(progress_, 1, wxALIGN_CENTER_VERTICAL | (allowCancel ? wxRIGHT : 0),
                  allowCancel ? 20 : 0);
    if (allowCancel) {
        auto* cancel = new CancelButton(panel, localization_.Text("break.cancel"));
        controls->Add(cancel, 0, wxALIGN_CENTER_VERTICAL);
        cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Finish(); });
    }
    layout->AddStretchSpacer(1);
    layout->Add(controls, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 18);
    panel->SetSizer(layout);
    auto* frameLayout = new wxBoxSizer(wxVERTICAL);
    frameLayout->Add(panel, 1, wxEXPAND);
    SetSizer(frameLayout);
    Bind(wxEVT_TIMER, &BreakWindow::OnTick, this, timer_.GetId());
    Bind(wxEVT_TIMER, &BreakWindow::OnFadeTick, this, fadeTimer_.GetId());
    UpdateRemaining();
    // Set opacity before showing the window to avoid a flash of solid black.
    const bool fade = CanSetTransparent() && SetTransparent(0);
#ifdef __WXOSX__
    // Do not use wxOSX's legacy ShowFullScreen(): hiding system UI there
    // depends on application activation and targets only the main screen.
    wxTheApp->Bind(wxEVT_ACTIVATE_APP, &BreakWindow::OnAppActivation, this);
    SetSize(geometry);
    ShowMacBreakWindow(*this);
#else
    ShowFullScreen(true, wxFULLSCREEN_ALL);
    Show();
    Raise();
#endif
    Layout();
    if (fade) {
        fadeStarted_ = std::chrono::steady_clock::now();
        if (fadeTimer_.Start(kFadeIntervalMs)) return;
        SetTransparent(255);
    }
    timer_.Start(1000);
}

BreakWindow::~BreakWindow() {
#ifdef __WXOSX__
    if (wxTheApp)
        wxTheApp->Unbind(wxEVT_ACTIVATE_APP, &BreakWindow::OnAppActivation, this);
#endif
}

#ifdef __WXOSX__
void BreakWindow::OnAppActivation(wxActivateEvent& event) {
    // wxOSX restores its own window levels before sending this event. Restore
    // the overlay level afterwards, including when a click activates the app.
    if (event.GetActive() && !finished_ && !IsBeingDeleted())
        KeepMacBreakWindowOnTop(*this);
    event.Skip();
}
#endif

void BreakWindow::OnFadeTick(wxTimerEvent&) {
    if (finished_) return;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - fadeStarted_).count();
    const double progress = std::min(1.0, static_cast<double>(elapsed) / kFadeDurationMs);
    const double eased = progress * progress * (3.0 - 2.0 * progress);
    const bool applied = SetTransparent(static_cast<wxByte>(255.0 * eased));
    if (progress >= 1.0 || !applied) {
        fadeTimer_.Stop();
        SetTransparent(255);
        // Keep the full configured rest duration after the fade completes.
        timer_.Start(1000);
    }
}

void BreakWindow::OnTick(wxTimerEvent&) {
    if (finished_) return;
    if (--remainingSeconds_ <= 0) Finish();
    else UpdateRemaining();
}

void BreakWindow::UpdateRemaining() {
    const int minutes = remainingSeconds_ / 60;
    const int seconds = remainingSeconds_ % 60;
    remainingLabel_->SetLabel(wxString::Format("%02d:%02d", minutes, seconds));
    progress_->Refresh();
}

void BreakWindow::Finish() {
    if (finished_) return;
    finished_ = true;
    timer_.Stop();
    fadeTimer_.Stop();
    if (onFinished_) onFinished_();
}
