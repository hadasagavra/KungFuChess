#pragma once

#include <cstdint>

#include "model/include/position.hpp"

namespace kfc::realtime {

// Pure data: a piece's planned/in-flight move across the board, with how long
// the traversal takes and how far along it is. Data only -- no logic, helpers,
// or calculations. Travel-time and progress updates live in the realtime
// mechanics (motion helpers / RealTimeArbiter), not in this container.
struct Motion {
    std::uint32_t pieceId;
    model::Position source;
    model::Position destination;
    int elapsedMs=0;
    int durationMs;
};

}  // namespace kfc::realtime
