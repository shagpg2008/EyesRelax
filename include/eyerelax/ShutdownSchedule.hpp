#pragma once

// The interval is [start, end). An end earlier than start crosses midnight.
// Equal endpoints deliberately mean an empty (disabled) interval.
bool IsWithinShutdownWindow(int secondsSinceMidnight, int startSeconds,
                            int endSeconds);
