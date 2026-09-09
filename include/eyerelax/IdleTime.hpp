#pragma once

#include <chrono>
#include <optional>

// Returns the time since the last system-wide keyboard or mouse input.
// std::nullopt means that the current platform cannot provide this information.
std::optional<std::chrono::milliseconds> GetSystemIdleTime();
