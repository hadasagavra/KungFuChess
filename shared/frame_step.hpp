#pragma once

#include <algorithm>
#include <chrono>

namespace kfc {

// The real-time step between two frames, in milliseconds, clamped to [0, maxMs].
// Both composition-root loops -- the client's window loop and the server's
// poll/tick loop -- advance their clock this way, so a paused or janky frame (or
// the first one) cannot jump the game clock forward. One definition, shared.
inline int clampedStepMs(std::chrono::steady_clock::time_point last,
                         std::chrono::steady_clock::time_point now, int maxMs) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    return std::clamp(static_cast<int>(ms), 0, maxMs);
}

}  // namespace kfc
