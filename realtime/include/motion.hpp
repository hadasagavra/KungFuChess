#pragma once

#include <cstdint>
#include <memory>

#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::realtime {

constexpr int cellSizePx = 100;
constexpr int pieceSpeedPxPerSec = 100;
constexpr int squareTravelMs = cellSizePx * 1000 / pieceSpeedPxPerSec;  // 1000
constexpr int jumpDurationMs = 1000;
constexpr int cooldownMs = 1000;

// Travel time for a slide: cell-step (Chebyshev) count times squareTravelMs.
int travelDurationMs(model::Position from, model::Position to);

// A read-only snapshot of one in-flight slide: where it started, where it is
// bound, and how far along it is (0..1). It carries no behavior, so the display
// can read a motion's progress without touching the live, mutating Motion.
struct MotionState {
    model::Position from;
    model::Position to;
    double progress;
};

// A read-only snapshot of one active cooldown: the cell of the resting piece and
// how far through its rest it is (0..1, 1 = done). Mirrors MotionState so the
// display can render a cooldown indicator without touching the live Cooldown.
struct CooldownState {
    model::Position cell;
    double progress;
};

// A piece sliding from one cell to another over time.
class Motion {
public:
    Motion(model::Position from, model::Position to);

    model::Position from() const { return from_; }
    model::Position to() const { return to_; }
    int durationMs() const { return durationMs_; }
    int elapsedMs() const { return elapsedMs_; }

    // Fraction of the slide completed, clamped to [0, 1].
    double progress() const;
    // A frozen, read-only view of this motion for callers outside realtime.
    MotionState state() const { return {from_, to_, progress()}; }

    void advance(int deltaMs);
    bool hasArrived() const { return elapsedMs_ >= durationMs_; }

private:
    model::Position from_;
    model::Position to_;
    int durationMs_;
    int elapsedMs_ = 0;
};

// A piece jumping in place: it stays on its cell for the jump window. If an enemy
// motion arrives during the window, the airborne piece is "lifted" out and the
// arriving enemy is captured when the jump lands.
class Jump {
public:
    explicit Jump(model::Position cell);

    model::Position cell() const { return cell_; }

    void advance(int deltaMs);
    bool hasLanded() const { return elapsedMs_ >= jumpDurationMs; }

    bool isLifted() const { return lifted_ != nullptr; }
    void lift(std::shared_ptr<model::Piece> piece) { lifted_ = std::move(piece); }
    std::shared_ptr<model::Piece> lifted() const { return lifted_; }

private:
    model::Position cell_;
    int elapsedMs_ = 0;
    std::shared_ptr<model::Piece> lifted_;
};

// A piece resting (cooldown) after a move or jump before it may act again.
class Cooldown {
public:
    Cooldown(std::uint32_t pieceId, model::Position cell);

    std::uint32_t pieceId() const { return pieceId_; }
    model::Position cell() const { return cell_; }

    // Fraction of the rest completed, clamped to [0, 1].
    double progress() const;
    // A frozen, read-only view of this cooldown for callers outside realtime.
    CooldownState state() const { return {cell_, progress()}; }

    void advance(int deltaMs);
    bool hasElapsed() const { return elapsedMs_ >= cooldownMs; }

private:
    std::uint32_t pieceId_;
    model::Position cell_;
    int elapsedMs_ = 0;
};

}  // namespace kfc::realtime
