#include "realtime/include/motion.hpp"

#include <cstdlib>

namespace kfc::realtime {

namespace {

int cellSteps(model::Position from, model::Position to) {
    const int dRow = std::abs(to.row - from.row);
    const int dCol = std::abs(to.col - from.col);
    return dRow > dCol ? dRow : dCol;
}

}  // namespace

int travelDurationMs(model::Position from, model::Position to) {
    return cellSteps(from, to) * squareTravelMs;
}

Motion::Motion(model::Position from, model::Position to)
    : from_(from), to_(to), durationMs_(travelDurationMs(from, to)) {}

void Motion::advance(int deltaMs) { elapsedMs_ += deltaMs; }

Jump::Jump(model::Position cell) : cell_(cell) {}

void Jump::advance(int deltaMs) { elapsedMs_ += deltaMs; }

Cooldown::Cooldown(std::uint32_t pieceId, model::Position cell)
    : pieceId_(pieceId), cell_(cell) {}

void Cooldown::advance(int deltaMs) { elapsedMs_ += deltaMs; }

}  // namespace kfc::realtime
