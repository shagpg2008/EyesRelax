#pragma once

#include <map>
#include <wx/string.h>

class Localization {
public:
    bool Load(const wxString& requestedLocale);
    wxString Text(const wxString& key) const;
    wxString Format(const wxString& key, const wxString& name,
                    const wxString& value) const;
    const wxString& Locale() const { return locale_; }

private:
    static wxString NormalizeLocale(const wxString& locale);
    static bool LoadFile(const wxString& path,
                         std::map<wxString, wxString>& destination);
    std::map<wxString, wxString> fallback_;
    std::map<wxString, wxString> current_;
    wxString locale_{"en"};
};

