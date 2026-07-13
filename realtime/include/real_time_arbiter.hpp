#pragma once

#include <cstdint>
#include <vector>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"

namespace kfc::realtime {

// Result of advancing time: aggregates whether any piece that arrived during the
// call captured a King, so the caller (GameEngine) can react without the arbiter
// ever knowing about GameEngine (no circular dependency).
struct ArbiterResult {
    bool kingCaptured = false;
};

// Owns the active Motions (kept entirely outside the Board) and advances them as
// time passes. It accepts only pre-validated raw moves, mutates the borrowed
// Board only at the atomic instant of arrival, and has zero knowledge of game
// legality, rendering, or input. Multiple pieces may be in flight at once; a
// single piece may not (startMotion enforces one active motion per piece).
class RealTimeArbiter {
public:
    explicit RealTimeArbiter(model::Board& board);

    // Computes durationMs from the move geometry and begins a motion. Throws
    // std::logic_error if this piece already has an active motion, and
    // std::invalid_argument if source holds no piece with the given id.
    void startMotion(std::uint32_t pieceId, model::Position source,
                     model::Position destination);

    // Advances every active motion by ms and resolves any arrivals atomically.
    ArbiterResult advanceTime(int ms);

    bool hasActiveMotion() const;

private:
    static int computeDurationMs(model::Position source, model::Position destination);
    bool hasMotionForPiece(std::uint32_t pieceId) const;
    bool resolveArrival(const Motion& motion);  // returns whether a King was captured

    model::Board& board_;
    std::vector<Motion> active_;
};

}  // namespace kfc::realtime
