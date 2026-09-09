#include "eyerelax/Settings.hpp"

#include <algorithm>
#include <wx/fileconf.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif

namespace {
wxString ConfigPath() {
    const wxString directory = wxStandardPaths::Get().GetUserConfigDir() +
                               wxFILE_SEP_PATH + "EyesRelax";
    wxFileName::Mkdir(directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return directory + wxFILE_SEP_PATH + "settings.ini";
}

wxString XmlEscape(wxString value) {
    value.Replace("&", "&amp;");
    value.Replace("\"", "&quot;");
    value.Replace("<", "&lt;");
    value.Replace(">", "&gt;");
    return value;
}

bool WriteUtf8File(const wxString& path, const wxString& contents) {
    wxFile file(path, wxFile::write);
    return file.IsOpened() && file.Write(contents, wxConvUTF8);
}

bool ApplyLaunchAtLogin(bool enabled) {
    const wxString executable = wxStandardPaths::Get().GetExecutablePath();
#ifdef __WXMSW__
    wxRegKey key(wxRegKey::HKCU, "Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    if (enabled) {
        if (!key.Exists() && !key.Create()) return false;
        return key.SetValue("EyesRelax", "\"" + executable + "\"");
    }
    return !key.Exists() || !key.HasValue("EyesRelax") || key.DeleteValue("EyesRelax");
#elif defined(__WXOSX__)
    const wxString directory = wxGetHomeDir() + wxFILE_SEP_PATH + "Library" +
                               wxFILE_SEP_PATH + "LaunchAgents";
    const wxString path = directory + wxFILE_SEP_PATH + "net.eyesrelax.app.plist";
    if (!enabled) return !wxFileExists(path) || wxRemoveFile(path);
    if (!wxFileName::Mkdir(directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) return false;
    const wxString plist =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\"><dict>\n"
        "<key>Label</key><string>net.eyesrelax.app</string>\n"
        "<key>ProgramArguments</key><array><string>" + XmlEscape(executable) +
        "</string></array>\n<key>RunAtLoad</key><true/>\n"
        "</dict></plist>\n";
    return WriteUtf8File(path, plist);
#else
    const wxString directory = wxStandardPaths::Get().GetUserConfigDir() +
                               wxFILE_SEP_PATH + "autostart";
    const wxString path = directory + wxFILE_SEP_PATH + "eyes-relax.desktop";
    if (!enabled) return !wxFileExists(path) || wxRemoveFile(path);
    if (!wxFileName::Mkdir(directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) return false;
    wxString escaped = executable;
    escaped.Replace("\\", "\\\\");
    escaped.Replace("\"", "\\\"");
    const wxString desktop =
        "[Desktop Entry]\nType=Application\nName=Eyes Relax\nExec=\"" + escaped +
        "\"\nTerminal=false\nX-GNOME-Autostart-enabled=true\n";
    return WriteUtf8File(path, desktop);
#endif
}
}

Settings Settings::Load() {
    Settings value;
    wxFileConfig config("EyesRelax", wxEmptyString, ConfigPath(), wxEmptyString,
                        wxCONFIG_USE_LOCAL_FILE);
    config.Read("shortWorkMinutes", &value.shortWorkMinutes, 20);
    config.Read("shortBreakSeconds", &value.shortBreakSeconds, 20);
    if (!config.Read("shortBreaksBeforeLong", &value.shortBreaksBeforeLong)) {
        int legacyLongWorkMinutes = 60;
        config.Read("longWorkMinutes", &legacyLongWorkMinutes, 60);
        value.shortBreaksBeforeLong =
            std::max(1, legacyLongWorkMinutes / std::max(1, value.shortWorkMinutes));
    }
    config.Read("longBreakSeconds", &value.longBreakSeconds, 300);
    config.Read("locale", &value.locale, "system");
    config.Read("launchAtLogin", &value.launchAtLogin, false);
    config.Read("allowBreakCancel", &value.allowBreakCancel, true);
    return value;
}

bool Settings::Save() const {
    wxFileConfig config("EyesRelax", wxEmptyString, ConfigPath(), wxEmptyString,
                        wxCONFIG_USE_LOCAL_FILE);
    config.Write("shortWorkMinutes", shortWorkMinutes);
    config.Write("shortBreakSeconds", shortBreakSeconds);
    config.Write("shortBreaksBeforeLong", shortBreaksBeforeLong);
    config.DeleteEntry("longWorkMinutes", false);
    config.Write("longBreakSeconds", longBreakSeconds);
    config.Write("locale", locale);
    config.Write("launchAtLogin", launchAtLogin);
    config.Write("allowBreakCancel", allowBreakCancel);
    return config.Flush() && ApplyLaunchAtLogin(launchAtLogin);
}
