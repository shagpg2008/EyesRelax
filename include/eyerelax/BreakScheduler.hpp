#pragma once

#include <chrono>

class BreakScheduler {
public:
    enum class BreakKind { Short, Long };

    BreakScheduler(int shortMinutes, int shortBreaksBeforeLong);
    void Configure(int shortMinutes, int shortBreaksBeforeLong);
    void Reset();
    void Pause(bool paused);
    bool IsPaused() const { return paused_; }
    bool IsDue() const;
    BreakKind DueKind() const;
    void Complete(BreakKind kind);
    std::chrono::seconds Remaining() const;

private:
    using Clock = std::chrono::steady_clock;
    std::chrono::minutes shortInterval_{20};
    int shortBreaksBeforeLong_{3};
    int completedShortBreaks_{0};
    Clock::time_point nextBreak_;
    bool paused_{false};
    Clock::time_point pausedAt_;
};
