#pragma once

#include <functional>
#include <wx/taskbar.h>

class Localization;

class TrayIcon final : public wxTaskBarIcon {
public:
    TrayIcon(const Localization& localization,
             std::function<void()> showSettings,
             std::function<void()> togglePause,
             std::function<void()> takeBreak,
             std::function<void()> quit,
             std::function<bool()> isPaused);

    wxMenu* CreatePopupMenu() override;

private:
    void UpdateIcon();

    const Localization& localization_;
    std::function<void()> showSettings_;
    std::function<void()> togglePause_;
    std::function<void()> takeBreak_;
    std::function<void()> quit_;
    std::function<bool()> isPaused_;
};
