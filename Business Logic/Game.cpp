#include "Game.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "MoveRules.h"

namespace kfc::logic {

namespace {

constexpr long long kSquareTravelMs = 1000;

int chebyshevDistance(Position from, Position to) {
    return std::max(std::abs(to.row - from.row), std::abs(to.col - from.col));
}

long long travelDurationMs(Position from, Position to) {
    return chebyshevDistance(from, to) * kSquareTravelMs;
}

}  // namespace

Game::Game(Board board) : board_(std::move(board)) {}

const Board& Game::board() const {
    return board_;
}

bool Game::isPieceInTransitAt(Position p) const {
    return pending_.has_value() && pending_->from == p;
}

bool Game::hasPieceInTransit() const {
    return pending_.has_value();
}

void Game::handleClickCell(Position p) {
    if (!board_.inBounds(p)) return;

    if (!selected_.has_value()) {
        trySelect(p);
        return;
    }

    if (tryReselectSameColor(p)) return;

    tryRequestMove(p);
}

void Game::trySelect(Position p) {
    if (!board_.isEmpty(p) && !isPieceInTransitAt(p)) {
        selected_ = p;
    }
}

bool Game::tryReselectSameColor(Position p) {
    if (board_.isEmpty(p) || isPieceInTransitAt(p) || !board_.sameColor(*selected_, p)) {
        return false;
    }
    selected_ = p;
    return true;
}

void Game::tryRequestMove(Position p) {
    if (hasPieceInTransit()) return;

    PieceType piece = board_.pieceTypeAt(*selected_);
    char color = board_.colorAt(*selected_);
    if (!isMoveLegal(board_, piece, color, *selected_, p)) return;

    pending_ = PendingMove{*selected_, p, clockMs_ + travelDurationMs(*selected_, p)};
    selected_.reset();
}

void Game::advanceClock(long long ms) {
    clockMs_ += ms;
    applyArrivedMove();
}

void Game::applyArrivedMove() {
    if (pending_.has_value() && pending_->arrivalMs <= clockMs_) {
        board_.movePiece(pending_->from, pending_->to);
        pending_.reset();
    }
}

}  // namespace kfc::logic