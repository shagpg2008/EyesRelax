#include "eyerelax/IdleTime.hpp"

#ifdef __WXMSW__
#include <windows.h>
#elif defined(__WXOSX__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(EYERELAX_HAVE_XSS)
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#endif

std::optional<std::chrono::milliseconds> GetSystemIdleTime() {
#ifdef __WXMSW__
    LASTINPUTINFO lastInput{};
    lastInput.cbSize = sizeof(lastInput);
    if (!GetLastInputInfo(&lastInput)) return std::nullopt;
    const DWORD elapsed = GetTickCount() - lastInput.dwTime;
    return std::chrono::milliseconds(elapsed);
#elif defined(__WXOSX__)
    const double seconds = CGEventSourceSecondsSinceLastEventType(
        kCGEventSourceStateCombinedSessionState, kCGAnyInputEventType);
    if (seconds < 0.0) return std::nullopt;
    return std::chrono::milliseconds(static_cast<long long>(seconds * 1000.0));
#elif defined(EYERELAX_HAVE_XSS)
    static Display* display = XOpenDisplay(nullptr);
    if (!display) return std::nullopt;
    XScreenSaverInfo* info = XScreenSaverAllocInfo();
    if (!info) return std::nullopt;
    const bool ok = XScreenSaverQueryInfo(display, DefaultRootWindow(display), info) != 0;
    const auto elapsed = std::chrono::milliseconds(info->idle);
    XFree(info);
    return ok ? std::optional<std::chrono::milliseconds>(elapsed) : std::nullopt;
#else
    return std::nullopt;
#endif
}
