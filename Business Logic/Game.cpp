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
    for (const PendingMove& move : activeMoves_) {
        if (move.from == p) return true;
    }
    return false;
}

void Game::handleClickCell(Position p) {
    if (gameOver_) return;

    applyArrivedMoves();

    if (!board_.inBounds(p)) return;
    if (isPieceInTransitAt(p)) return;

    if (!board_.isEmpty(p)) {
        if (selected_.has_value() && !board_.sameColor(*selected_, p)) {
            tryRequestMove(p);
        } else {
            selected_ = p;
        }
    } else if (selected_.has_value()) {
        tryRequestMove(p);
    }
}

void Game::tryRequestMove(Position to) {
    PieceType piece = board_.pieceTypeAt(*selected_);
    char color = board_.colorAt(*selected_);
    if (!isMoveLegal(board_, piece, color, *selected_, to)) return;

    activeMoves_.push_back(PendingMove{*selected_, to, clockMs_ + travelDurationMs(*selected_, to)});
    selected_.reset();
}

void Game::advanceClock(long long ms) {
    clockMs_ += ms;
    applyArrivedMoves();
}

bool Game::canLand(Position from, Position to) const {
    if (board_.isEmpty(from)) return false;

    PieceType piece = board_.pieceTypeAt(from);
    char color = board_.colorAt(from);
    if (!isMoveLegal(board_, piece, color, from, to)) return false;

    if (!board_.isEmpty(to) && board_.colorAt(to) == color) return false;

    return true;
}

void Game::applyArrivedMoves() {
    bool changed = true;
    while (changed && !gameOver_) {
        changed = false;
        for (auto it = activeMoves_.begin(); it != activeMoves_.end(); ++it) {
            if (it->arrivalMs > clockMs_) continue;

            Position from = it->from;
            Position to = it->to;
            activeMoves_.erase(it);

            if (canLand(from, to)) {
                if (board_.isKing(to)) gameOver_ = true;
                board_.movePiece(from, to);
            }

            changed = true;
            break;
        }
    }
}

}  // namespace kfc::logic