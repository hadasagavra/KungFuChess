#include "shared/logic/realtime/include/motion.hpp"

#include <algorithm>
#include <cstdlib>

namespace kfc::realtime {

namespace {

int cellSteps(model::Position from, model::Position to) {
    const int dRow = std::abs(to.row - from.row);
    const int dCol = std::abs(to.col - from.col);
    return dRow > dCol ? dRow : dCol;
}

// A move is walked cell by cell only when it runs along a rank, a file, or a
// true diagonal; anything else is a leap over the squares in between.
bool isStraightLine(model::Position from, model::Position to) {
    const int dRow = std::abs(to.row - from.row);
    const int dCol = std::abs(to.col - from.col);
    return dRow == 0 || dCol == 0 || dRow == dCol;
}

int directionOf(int delta) { return (delta > 0) - (delta < 0); }

// Time for one crossing between adjacent cells. Spreading the move's total
// travel time over its crossings keeps every piece's overall speed exactly as
// it was: a straight move pays squareTravelMs per cell, and a leap pays its
// whole travel time for the single crossing it makes.
int stepDurationOf(model::Position from, model::Position to,
                   std::size_t pathLength) {
    if (pathLength < 2) {
        return 0;
    }
    return travelDurationMs(from, to) / static_cast<int>(pathLength - 1);
}

}  // namespace

int travelDurationMs(model::Position from, model::Position to) {
    return cellSteps(from, to) * squareTravelMs;
}

std::vector<model::Position> travelPath(model::Position from,
                                        model::Position to) {
    std::vector<model::Position> path{from};
    if (from == to) {
        return path;
    }
    if (!isStraightLine(from, to)) {
        path.push_back(to);
        return path;
    }

    const int rowStep = directionOf(to.row - from.row);
    const int colStep = directionOf(to.col - from.col);
    model::Position current = from;
    while (current != to) {
        current = model::Position{current.row + rowStep, current.col + colStep};
        path.push_back(current);
    }
    return path;
}

Motion::Motion(model::Position from, model::Position to)
    : path_(travelPath(from, to)),
      stepDurationMs_(stepDurationOf(from, to, path_.size())) {}

double Motion::stepProgress() const {
    if (stepDurationMs_ <= 0) {
        return 1.0;
    }
    const double fraction =
        static_cast<double>(elapsedInStepMs_) / stepDurationMs_;
    return std::clamp(fraction, 0.0, 1.0);
}

MotionState Motion::state() const {
    const model::Position current = currentCell();
    return {current, isComplete() ? current : nextCell(), stepProgress()};
}

void Motion::completeStep() {
    ++stepIndex_;
    elapsedInStepMs_ -= stepDurationMs_;
}

void Motion::stopHere() {
    path_.resize(stepIndex_ + 1);
    elapsedInStepMs_ = 0;
}

void Motion::advance(int deltaMs) { elapsedInStepMs_ += deltaMs; }

Jump::Jump(model::Position cell) : cell_(cell) {}

void Jump::advance(int deltaMs) { elapsedMs_ += deltaMs; }

Cooldown::Cooldown(std::uint32_t pieceId, model::Position cell)
    : pieceId_(pieceId), cell_(cell) {}

double Cooldown::progress() const {
    if constexpr (cooldownMs <= 0) {
        return 1.0;
    } else {
        const double fraction = static_cast<double>(elapsedMs_) / cooldownMs;
        return std::clamp(fraction, 0.0, 1.0);
    }
}

void Cooldown::advance(int deltaMs) { elapsedMs_ += deltaMs; }

}  // namespace kfc::realtime
