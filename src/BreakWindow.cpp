#include "eyerelax/BreakWindow.hpp"

#include "eyerelax/Localization.hpp"
#include <algorithm>
#include <wx/control.h>
#include <wx/dcbuffer.h>
#include <wx/gauge.h>
#include <wx/mousestate.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {
class CancelButton final : public wxControl {
public:
    CancelButton(wxWindow* parent, const wxString& label)
        : wxControl(parent, wxID_CANCEL, wxDefaultPosition, wxSize(100, 42),
                    wxBORDER_NONE | wxWANTS_CHARS), label_(label) {
        SetMinSize(wxSize(100, 42));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        Bind(wxEVT_PAINT, &CancelButton::OnPaint, this);
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
        dc.DrawCircle(18, centreY, 13);
        dc.SetPen(wxPen(wxColour(255, 255, 255), 2));
        dc.DrawLine(13, centreY - 5, 23, centreY + 5);
        dc.DrawLine(23, centreY - 5, 13, centreY + 5);

        dc.SetFont(GetFont());
        dc.SetTextForeground(wxColour(25, 25, 25));
        dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
        int textWidth = 0, textHeight = 0;
        dc.GetTextExtent(label_, &textWidth, &textHeight);
        const int textAreaLeft = 34;
        dc.DrawText(label_, textAreaLeft + (bounds.width - textAreaLeft - textWidth) / 2,
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
      onFinished_(std::move(onFinished)), timer_(this) {
    const wxColour background(0, 0, 0);
    SetBackgroundColour(background);
    auto* panel = new wxPanel(this);
    panel->SetBackgroundColour(background);
    auto* layout = new wxBoxSizer(wxVERTICAL);
    auto* controls = new wxBoxSizer(wxHORIZONTAL);

    remainingLabel_ = new wxStaticText(
        panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(120, 42),
        wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE | wxBORDER_SIMPLE);
    remainingLabel_->SetBackgroundColour(wxColour(255, 255, 255));
    remainingLabel_->SetForegroundColour(wxColour(80, 80, 80));
    wxFont timeFont = remainingLabel_->GetFont();
    timeFont.SetPointSize(20);
    remainingLabel_->SetFont(timeFont);

    progress_ = new wxGauge(panel, wxID_ANY, totalSeconds_, wxDefaultPosition,
                            wxSize(-1, 42), wxGA_HORIZONTAL | wxGA_SMOOTH);
    progress_->SetForegroundColour(wxColour(0, 128, 0));
    progress_->SetBackgroundColour(wxColour(225, 225, 225));

    controls->Add(remainingLabel_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
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
    Bind(wxEVT_TIMER, &BreakWindow::OnTick, this);
    UpdateRemaining();
    timer_.Start(1000);
    ShowFullScreen(true, wxFULLSCREEN_ALL);
}

void BreakWindow::OnTick(wxTimerEvent&) {
    if (--remainingSeconds_ <= 0) Finish();
    else UpdateRemaining();
}

void BreakWindow::UpdateRemaining() {
    const int minutes = remainingSeconds_ / 60;
    const int seconds = remainingSeconds_ % 60;
    remainingLabel_->SetLabel(wxString::Format("%02d:%02d", minutes, seconds));
    progress_->SetValue(remainingSeconds_);
}

void BreakWindow::Finish() {
    if (finished_) return;
    finished_ = true;
    timer_.Stop();
    if (onFinished_) onFinished_();
}
