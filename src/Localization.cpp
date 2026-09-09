#include "eyerelax/Localization.hpp"

#include <cctype>
#include <sstream>
#include <vector>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace {
void SkipSpace(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

bool ParseString(const std::string& s, std::size_t& i, std::string& out) {
    SkipSpace(s, i);
    if (i >= s.size() || s[i++] != '"') return false;
    out.clear();
    while (i < s.size()) {
        const char c = s[i++];
        if (c == '"') return true;
        if (c != '\\') { out += c; continue; }
        if (i >= s.size()) return false;
        const char e = s[i++];
        switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default: return false;
        }
    }
    return false;
}
}

wxString Localization::NormalizeLocale(const wxString& input) {
    wxString locale = input;
    locale.Replace("_", "-");
    const int dot = locale.Find('.');
    if (dot != wxNOT_FOUND) locale = locale.Left(dot);
    const wxString lower = locale.Lower();
    if (lower.StartsWith("zh-cn") || lower.StartsWith("zh-sg") ||
        lower.StartsWith("zh-hans") || lower.StartsWith("zh-chs") || lower == "zh") {
        return "zh-Hans";
    }
    if (lower.StartsWith("zh-tw") || lower.StartsWith("zh-hk") ||
        lower.StartsWith("zh-mo") || lower.StartsWith("zh-hant") ||
        lower.StartsWith("zh-cht")) {
        return "zh-Hant";
    }
    if (lower.StartsWith("pt-br")) return "pt-BR";
    const int dash = locale.Find('-');
    return dash == wxNOT_FOUND ? locale.Lower() : locale.Left(dash).Lower();
}

bool Localization::LoadFile(const wxString& path,
                            std::map<wxString, wxString>& destination) {
    if (!wxFileExists(path)) return false;
    wxFile input(path, wxFile::read);
    if (!input.IsOpened()) return false;
    const wxFileOffset length = input.Length();
    if (length <= 0 || length > 1024 * 1024) return false;
    std::string json(static_cast<std::size_t>(length), '\0');
    if (input.Read(&json[0], static_cast<std::size_t>(length)) != length) return false;
    std::size_t i = 0;
    SkipSpace(json, i);
    if (i >= json.size() || json[i++] != '{') return false;
    while (true) {
        SkipSpace(json, i);
        if (i < json.size() && json[i] == '}') return true;
        std::string key, value;
        if (!ParseString(json, i, key)) return false;
        SkipSpace(json, i);
        if (i >= json.size() || json[i++] != ':') return false;
        if (!ParseString(json, i, value)) return false;
        destination[wxString::FromUTF8(key)] = wxString::FromUTF8(value);
        SkipSpace(json, i);
        if (i < json.size() && json[i] == ',') { ++i; continue; }
        if (i < json.size() && json[i] == '}') return true;
        return false;
    }
}

bool Localization::Load(const wxString& requested) {
    wxString base = wxStandardPaths::Get().GetExecutablePath();
    base = wxFileName(base).GetPath() + wxFILE_SEP_PATH + "locales" + wxFILE_SEP_PATH;
    fallback_.clear();
    current_.clear();
    if (!LoadFile(base + "en.json", fallback_)) {
        base = wxFileName(wxGetCwd(), "locales").GetFullPath() + wxFILE_SEP_PATH;
        if (!LoadFile(base + "en.json", fallback_)) return false;
    }
    wxString selected = requested;
    if (selected == "system") {
        selected = wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage());
    }
    locale_ = NormalizeLocale(selected);
    if (locale_ != "en" && !LoadFile(base + locale_ + ".json", current_)) locale_ = "en";
    return true;
}

wxString Localization::Text(const wxString& key) const {
    auto found = current_.find(key);
    if (found != current_.end()) return found->second;
    found = fallback_.find(key);
    return found != fallback_.end() ? found->second : key;
}

wxString Localization::Format(const wxString& key, const wxString& name,
                              const wxString& value) const {
    wxString result = Text(key);
    result.Replace("{" + name + "}", value);
    return result;
}
