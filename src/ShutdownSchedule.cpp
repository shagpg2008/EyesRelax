#include "eyerelax/ShutdownSchedule.hpp"

namespace {
int Normalize(int value) {
    value %= 24 * 60 * 60;
    return value < 0 ? value + 24 * 60 * 60 : value;
}
}

bool IsWithinShutdownWindow(int secondsSinceMidnight, int startSeconds,
                            int endSeconds) {
    const int now = Normalize(secondsSinceMidnight);
    const int start = Normalize(startSeconds);
    const int end = Normalize(endSeconds);
    if (start == end) return false;
    if (start < end) return now >= start && now < end;
    return now >= start || now < end;
}
