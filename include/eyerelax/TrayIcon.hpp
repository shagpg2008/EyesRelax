#pragma once

#include <functional>
#include <wx/string.h>
#ifndef EYERELAX_USE_APPINDICATOR
#include <wx/icon.h>
#include <wx/bmpbndl.h>
#include <wx/taskbar.h>
#endif

class Localization;

#ifdef EYERELAX_USE_APPINDICATOR
class TrayIcon final {
#else
class TrayIcon final : public wxTaskBarIcon {
#endif
public:
    TrayIcon(const Localization& localization,
             std::function<void()> showSettings,
             std::function<void()> togglePause,
             std::function<void()> takeBreak,
             std::function<void()> quit,
             std::function<bool()> isPaused);
    ~TrayIcon();
    void SetStatus(const wxString& status);

#ifndef EYERELAX_USE_APPINDICATOR
    wxMenu* CreatePopupMenu() override;
#endif

private:
    void UpdateIcon();
#ifdef EYERELAX_USE_APPINDICATOR
    static void OnShowActivated(void* item, void* instance);
    static void OnPauseActivated(void* item, void* instance);
    static void OnBreakActivated(void* item, void* instance);
    static void OnQuitActivated(void* item, void* instance);
    void* indicator_{nullptr};
    void* menu_{nullptr};
    void* pauseItem_{nullptr};
    void* statusItem_{nullptr};
#else
    wxIcon openIcon_;
    wxIcon closedIcon_;
#endif
    wxString status_;

    const Localization& localization_;
    std::function<void()> showSettings_;
    std::function<void()> togglePause_;
    std::function<void()> takeBreak_;
    std::function<void()> quit_;
    std::function<bool()> isPaused_;
};
