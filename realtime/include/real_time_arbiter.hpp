#pragma once

#include <cstdint>
#include <vector>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"

namespace kfc::realtime {

// What resolved this tick. landed = true for a normal motion arrival, false for
// a jump landing. kingCaptured flags a king being taken so the engine can end
// the game (the arbiter never knows about the engine).
struct ArrivalReport {
    model::Position destination;
    bool kingCaptured = false;
    bool landed = true;
};

// Owns the in-flight motions, jumps, and cooldowns outside the Board, and
// advances them as time passes. It mutates the borrowed Board only when a
// motion/jump resolves. It knows nothing of chess movement rules, rendering, or
// input; it only resolves occupancy over time.
class RealTimeArbiter {
public:
    explicit RealTimeArbiter(model::Board& board);

    bool hasActiveMotion() const { return !active_.empty(); }

    // A read-only view of the in-flight slides, for callers (e.g. the display)
    // that need each motion's from/to/progress without seeing the live Motions.
    std::vector<MotionState> activeMotions() const;

    // Begin a slide / a jump. Returns false if it cannot start.
    bool startMotion(model::Position from, model::Position to);
    bool startJump(model::Position cell);

    std::vector<ArrivalReport> advance(int deltaMs);

private:
    ArrivalReport resolveArrival(const Motion& motion);
    void landAirborne(int deltaMs, std::vector<ArrivalReport>& reports);
    void startCooldown(std::uint32_t pieceId, model::Position cell);
    void tickCooldowns(int deltaMs);
    bool isAirborneAt(model::Position cell) const;
    Jump* jumpAt(model::Position cell);

    model::Board& board_;
    std::vector<Motion> active_;
    std::vector<Jump> airborne_;
    std::vector<Cooldown> resting_;
};

}  // namespace kfc::realtime
