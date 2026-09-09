#include "eyerelax/TrayIcon.hpp"

#include "eyerelax/Icons.hpp"
#include "eyerelax/Localization.hpp"
#include <wx/menu.h>

namespace {
enum { ID_SHOW = wxID_HIGHEST + 1, ID_PAUSE, ID_BREAK };
}

TrayIcon::TrayIcon(const Localization& localization,
                   std::function<void()> showSettings,
                   std::function<void()> togglePause,
                   std::function<void()> takeBreak,
                   std::function<void()> quit,
                   std::function<bool()> isPaused)
    : localization_(localization), showSettings_(std::move(showSettings)),
      togglePause_(std::move(togglePause)), takeBreak_(std::move(takeBreak)),
      quit_(std::move(quit)), isPaused_(std::move(isPaused)) {
    UpdateIcon();
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { showSettings_(); }, ID_SHOW);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { togglePause_(); UpdateIcon(); }, ID_PAUSE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { takeBreak_(); }, ID_BREAK);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { quit_(); }, wxID_EXIT);
    Bind(wxEVT_TASKBAR_LEFT_DCLICK,
         [this](wxTaskBarIconEvent&) { togglePause_(); UpdateIcon(); });
}

void TrayIcon::UpdateIcon() {
    SetIcon(CreateEyeIcon(!isPaused_()), localization_.Text("app.name"));
}

wxMenu* TrayIcon::CreatePopupMenu() {
    auto* menu = new wxMenu;
    menu->Append(ID_SHOW, localization_.Text("tray.show"));
    menu->Append(ID_PAUSE, localization_.Text(isPaused_() ? "tray.resume" : "tray.pause"));
    menu->Append(ID_BREAK, localization_.Text("tray.takeBreak"));
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, localization_.Text("tray.quit"));
    return menu;
}
