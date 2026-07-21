#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::realtime {

constexpr int cellSizePx = 100;
constexpr int pieceSpeedPxPerSec = 100;
constexpr int squareTravelMs = cellSizePx * 1000 / pieceSpeedPxPerSec;  // 1000
constexpr int jumpDurationMs = 1000;
constexpr int cooldownMs = 1000;

// Travel time for a slide: cell-step (Chebyshev) count times squareTravelMs.
int travelDurationMs(model::Position from, model::Position to);

// The cells a slide passes through, first to last inclusive. A straight line
// (orthogonal or diagonal) is walked cell by cell, so every square in between is
// touched; any other shape is a hop that lands without touching what lies
// between (a knight leaps over obstructions). This is the one definition of
// "which squares a travelling piece occupies on the way" -- both the arbiter's
// collision handling and the display's slide read it from here.
std::vector<model::Position> travelPath(model::Position from,
                                        model::Position to);

// A read-only snapshot of one in-flight slide: the cell the piece occupies right
// now, the cell it is entering next, and how far into that single-cell step it
// is (0..1). It describes the current step rather than the whole journey, so a
// display can interpolate between two adjacent cells. It carries no behavior, so
// the display can read a motion's progress without touching the live, mutating
// Motion.
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

// A piece sliding from one cell to another over time, one cell at a time. The
// slide is modelled as a walk along travelPath: the piece occupies exactly one
// cell at any instant, and crossing into the next cell is an event the arbiter
// must approve (it may be a capture, or it may be blocked). Stepping like this
// is what lets the Business Logic know where a travelling piece actually is,
// rather than only where it started and where it is headed.
class Motion {
public:
    Motion(model::Position from, model::Position to);

    // The cell the piece occupies right now -- its cell on the Board.
    model::Position currentCell() const { return path_[stepIndex_]; }
    // The cell it is travelling into. Only meaningful while !isComplete().
    model::Position nextCell() const { return path_[stepIndex_ + 1]; }
    // The final cell of the journey, which stopHere() may bring nearer.
    model::Position destination() const { return path_.back(); }

    bool isComplete() const { return stepIndex_ + 1 >= path_.size(); }

    // The current step has run its course, so the piece is crossing into
    // nextCell() and the arbiter must decide what that entry means.
    bool isEnteringNextCell() const {
        return !isComplete() && elapsedInStepMs_ >= stepDurationMs_;
    }
    // How long ago that crossing happened, in ms. Two motions crossing during
    // the same tick did not really cross at the same instant: the larger
    // overshoot crossed earlier. This recovers that order.
    int arrivalOvershootMs() const { return elapsedInStepMs_ - stepDurationMs_; }

    // Commit the crossing: the piece now occupies what was nextCell(). Leftover
    // time carries into the next step, so a long tick crosses several cells
    // without the schedule drifting.
    void completeStep();
    // End the journey on the current cell (a piece stopped short of its target).
    void stopHere();

    // Fraction of the current single-cell step completed, clamped to [0, 1].
    double stepProgress() const;
    // A frozen, read-only view of this motion for callers outside realtime.
    MotionState state() const;

    void advance(int deltaMs);

private:
    std::vector<model::Position> path_;
    std::size_t stepIndex_ = 0;
    int stepDurationMs_;
    int elapsedInStepMs_ = 0;
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
