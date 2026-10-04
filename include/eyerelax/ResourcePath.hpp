#pragma once

#include <wx/filename.h>
#include <wx/stdpaths.h>

// Support macOS bundles, portable builds, and Linux resources under share/eyesrelax.
inline wxString ResourcePath(const wxString& relativePath) {
#ifdef __WXOSX__
    const wxString bundled = wxStandardPaths::Get().GetResourcesDir() +
                             wxFILE_SEP_PATH + relativePath;
    if (wxFileExists(bundled) || wxDirExists(bundled)) return bundled;
#endif
    const wxString executableDirectory =
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    const wxString portable = executableDirectory + wxFILE_SEP_PATH + relativePath;
    if (wxFileExists(portable) || wxDirExists(portable)) return portable;
#ifdef EYERELAX_RESOURCE_DIR_FROM_BIN
    wxFileName installed(executableDirectory + wxFILE_SEP_PATH +
                         wxString::FromUTF8(EYERELAX_RESOURCE_DIR_FROM_BIN) +
                         wxFILE_SEP_PATH + relativePath);
    installed.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    return installed.GetFullPath();
#else
    return portable;
#endif
}
