#include "eyerelax/PowerControl.hpp"

#ifdef __WXMSW__
#include <windows.h>

namespace {
bool EnableShutdownPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &token)) return false;
    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    const bool found = LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME,
                                             &privileges.Privileges[0].Luid) != 0;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    const bool adjusted = found && AdjustTokenPrivileges(
        token, FALSE, &privileges, 0, nullptr, nullptr) != 0 &&
        GetLastError() == ERROR_SUCCESS;
    CloseHandle(token);
    return adjusted;
}
}

bool RequestSystemShutdown() {
    if (!EnableShutdownPrivilege()) return false;
    return ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
                         SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER |
                             SHTDN_REASON_FLAG_PLANNED) != 0;
}
#else
#include <wx/utils.h>

bool RequestSystemShutdown() {
#ifdef __WXOSX__
    return wxExecute("osascript -e 'tell application \"System Events\" to shut down'",
                     wxEXEC_ASYNC) != 0;
#else
    return wxExecute("systemctl poweroff", wxEXEC_ASYNC) != 0;
#endif
}
#endif
