#include "eyerelax/TrayIcon.hpp"

#include "eyerelax/Icons.hpp"
#include "eyerelax/Localization.hpp"
#include "eyerelax/ResourcePath.hpp"
#include <wx/filename.h>
#include <wx/stdpaths.h>

#ifdef EYERELAX_USE_APPINDICATOR
#ifdef EYERELAX_USE_AYATANA_APPINDICATOR
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif
#include <gtk/gtk.h>
#else
#include <wx/menu.h>
#endif

#ifndef EYERELAX_USE_APPINDICATOR
namespace {
enum { ID_SHOW = wxID_HIGHEST + 1, ID_PAUSE, ID_BREAK };
}
#endif

TrayIcon::TrayIcon(const Localization& localization,
                   std::function<void()> showSettings,
                   std::function<void()> togglePause,
                   std::function<void()> takeBreak,
                   std::function<void()> quit,
                   std::function<bool()> isPaused)
    : localization_(localization), showSettings_(std::move(showSettings)),
      togglePause_(std::move(togglePause)), takeBreak_(std::move(takeBreak)),
      quit_(std::move(quit)), isPaused_(std::move(isPaused)) {
#ifdef EYERELAX_USE_APPINDICATOR
    const wxString iconPath = ResourcePath("icons/eye-open.png");
    indicator_ = app_indicator_new("eyes-relax", iconPath.utf8_str(),
                                    APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(static_cast<AppIndicator*>(indicator_), APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_title(static_cast<AppIndicator*>(indicator_),
                            localization_.Text("app.name").utf8_str());

    auto* menu = gtk_menu_new();
    menu_ = menu;
    statusItem_ = gtk_menu_item_new_with_label("");
    gtk_widget_set_sensitive(static_cast<GtkWidget*>(statusItem_), false);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), static_cast<GtkWidget*>(statusItem_));
    auto addItem = [menu, this](const wxString& label, GCallback callback) {
        GtkWidget* item = gtk_menu_item_new_with_label(label.utf8_str());
        g_signal_connect(item, "activate", callback, this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        return item;
    };
    addItem(localization_.Text("tray.show"), G_CALLBACK(&TrayIcon::OnShowActivated));
    pauseItem_ = addItem(localization_.Text("tray.pause"),
                         G_CALLBACK(&TrayIcon::OnPauseActivated));
    addItem(localization_.Text("tray.takeBreak"), G_CALLBACK(&TrayIcon::OnBreakActivated));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    addItem(localization_.Text("tray.quit"), G_CALLBACK(&TrayIcon::OnQuitActivated));
    gtk_widget_show_all(menu);
    app_indicator_set_menu(static_cast<AppIndicator*>(indicator_), GTK_MENU(menu));
#else
    openIcon_ = CreateEyeIcon(true);
    closedIcon_ = CreateEyeIcon(false);
    UpdateIcon();
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { showSettings_(); }, ID_SHOW);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { togglePause_(); UpdateIcon(); }, ID_PAUSE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { takeBreak_(); }, ID_BREAK);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { quit_(); }, wxID_EXIT);
    Bind(wxEVT_TASKBAR_LEFT_DCLICK,
         [this](wxTaskBarIconEvent&) { togglePause_(); UpdateIcon(); });
#endif
}

TrayIcon::~TrayIcon() {
#ifdef EYERELAX_USE_APPINDICATOR
    if (indicator_) {
        app_indicator_set_status(static_cast<AppIndicator*>(indicator_),
                                 APP_INDICATOR_STATUS_PASSIVE);
    }
    if (menu_) gtk_widget_destroy(static_cast<GtkWidget*>(menu_));
    if (indicator_) g_object_unref(indicator_);
#endif
}

void TrayIcon::SetStatus(const wxString& status) {
    if (status_ == status) return;
    status_ = status;
    UpdateIcon();
}

void TrayIcon::UpdateIcon() {
    const wxString tooltip = localization_.Text("app.name") +
                             (status_.empty() ? wxString() : "\n" + status_);
#ifdef EYERELAX_USE_APPINDICATOR
    const bool paused = isPaused_();
    const wxString iconPath = ResourcePath(paused ? "icons/eye-closed.png" : "icons/eye-open.png");
    app_indicator_set_icon_full(static_cast<AppIndicator*>(indicator_), iconPath.utf8_str(),
                                tooltip.utf8_str());
    gtk_menu_item_set_label(GTK_MENU_ITEM(statusItem_), status_.utf8_str());
    gtk_menu_item_set_label(GTK_MENU_ITEM(pauseItem_),
                            localization_.Text(paused ? "tray.resume" : "tray.pause").utf8_str());
#else
    SetIcon(isPaused_() ? closedIcon_ : openIcon_, tooltip);
#endif
}

#ifdef EYERELAX_USE_APPINDICATOR
void TrayIcon::OnShowActivated(void*, void* instance) {
    static_cast<TrayIcon*>(instance)->showSettings_();
}

void TrayIcon::OnPauseActivated(void*, void* instance) {
    auto* tray = static_cast<TrayIcon*>(instance);
    tray->togglePause_();
    tray->UpdateIcon();
}

void TrayIcon::OnBreakActivated(void*, void* instance) {
    static_cast<TrayIcon*>(instance)->takeBreak_();
}

void TrayIcon::OnQuitActivated(void*, void* instance) {
    static_cast<TrayIcon*>(instance)->quit_();
}
#else
wxMenu* TrayIcon::CreatePopupMenu() {
    auto* menu = new wxMenu;
    menu->Append(ID_SHOW, localization_.Text("tray.show"));
    menu->Append(ID_PAUSE, localization_.Text(isPaused_() ? "tray.resume" : "tray.pause"));
    menu->Append(ID_BREAK, localization_.Text("tray.takeBreak"));
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, localization_.Text("tray.quit"));
    return menu;
}
#endif
