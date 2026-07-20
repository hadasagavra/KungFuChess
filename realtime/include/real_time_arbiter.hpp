#pragma once

#include <cstddef>
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

// What it means for a travelling piece to cross into the cell ahead of it. Every
// encounter between pieces -- whether the other one is standing still, resting,
// or travelling too -- is decided here and only here, because a travelling piece
// occupies a real cell on the Board at every instant.
enum class EntryOutcome {
    Free,                // the cell is empty; walk in
    Capture,             // an enemy is there; the piece arriving later takes it
    BlockedByFriendly,   // a piece of the same colour is there; stop short of it
    CapturedByAirborne   // an airborne enemy holds the cell and takes the mover
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

    // A read-only view of the active cooldowns, for callers (e.g. the display)
    // that need each resting cell + progress without seeing the live Cooldowns.
    std::vector<CooldownState> activeCooldowns() const;

    // Begin a slide / a jump. Returns false if it cannot start. Motions of
    // different pieces run side by side; a piece already travelling cannot be
    // sent somewhere new until it stops.
    bool startMotion(model::Position from, model::Position to);
    bool startJump(model::Position cell);

    std::vector<ArrivalReport> advance(int deltaMs);

private:
    // Motions are resolved by index into active_, and finished ones are marked in
    // a `resolved` flag vector rather than erased on the spot, so indices stay
    // valid while one motion's outcome (a capture) ends another's.
    using ResolvedFlags = std::vector<bool>;

    bool hasMotionFor(model::Position cell) const;

    void advanceMotions(int deltaMs, std::vector<ArrivalReport>& reports);
    // The motions crossing into a new cell right now, ordered by the instant they
    // crossed -- earliest first, so that the piece arriving later takes the one
    // that got there before it.
    std::vector<std::size_t> crossingsByArrival(const ResolvedFlags& resolved) const;
    EntryOutcome classifyEntry(const Motion& motion) const;
    void resolveEntry(std::size_t index, ResolvedFlags& resolved,
                      std::vector<ArrivalReport>& reports);
    // Take the piece on `cell` off the board and stop it travelling. Returns
    // whether it was a king.
    bool captureAt(model::Position cell, ResolvedFlags& resolved);
    void yieldToAirborne(std::size_t index, ResolvedFlags& resolved,
                         std::vector<ArrivalReport>& reports);
    void finishMotion(std::size_t index, bool kingCaptured,
                      ResolvedFlags& resolved,
                      std::vector<ArrivalReport>& reports);
    void dropResolved(const ResolvedFlags& resolved);

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
