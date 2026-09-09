#include "eyerelax/BreakScheduler.hpp"

#include <algorithm>

BreakScheduler::BreakScheduler(int shortMinutes, int shortBreaksBeforeLong) {
    Configure(shortMinutes, shortBreaksBeforeLong);
}

void BreakScheduler::Configure(int shortMinutes, int shortBreaksBeforeLong) {
    shortInterval_ = std::chrono::minutes(std::max(1, shortMinutes));
    shortBreaksBeforeLong_ = std::max(1, shortBreaksBeforeLong);
    Reset();
}

void BreakScheduler::Reset() {
    const auto now = Clock::now();
    nextBreak_ = now + shortInterval_;
    completedShortBreaks_ = 0;
    paused_ = false;
}

void BreakScheduler::Pause(bool paused) {
    if (paused == paused_) return;
    if (paused) {
        pausedAt_ = Clock::now();
    } else {
        const auto delta = Clock::now() - pausedAt_;
        nextBreak_ += delta;
    }
    paused_ = paused;
}

bool BreakScheduler::IsDue() const {
    if (paused_) return false;
    const auto now = Clock::now();
    return now >= nextBreak_;
}

BreakScheduler::BreakKind BreakScheduler::DueKind() const {
    return completedShortBreaks_ >= shortBreaksBeforeLong_ ? BreakKind::Long
                                                           : BreakKind::Short;
}

void BreakScheduler::Complete(BreakKind kind) {
    if (kind == BreakKind::Long) {
        completedShortBreaks_ = 0;
    } else {
        ++completedShortBreaks_;
    }
    nextBreak_ = Clock::now() + shortInterval_;
}

std::chrono::seconds BreakScheduler::Remaining() const {
    const auto reference = paused_ ? pausedAt_ : Clock::now();
    if (reference >= nextBreak_) return std::chrono::seconds::zero();
    return std::chrono::duration_cast<std::chrono::seconds>(nextBreak_ - reference);
}
