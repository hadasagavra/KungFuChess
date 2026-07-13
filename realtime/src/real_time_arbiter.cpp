#include "realtime/include/real_time_arbiter.hpp"

#include <memory>
#include <stdexcept>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"

namespace kfc::realtime {

using model::Board;
using model::Kind;
using model::Piece;
using model::Position;
using model::State;

namespace {

constexpr int MsPerCell = 1000;

}  // namespace

RealTimeArbiter::RealTimeArbiter(Board& board) : board_(board) {}

int RealTimeArbiter::computeDurationMs(Position source, Position destination) {
    int dRow = destination.row - source.row;
    int dCol = destination.col - source.col;
    if (dRow < 0) dRow = -dRow;
    if (dCol < 0) dCol = -dCol;
    const int cells = dRow > dCol ? dRow : dCol;  // Chebyshev: cell-step count
    return cells * MsPerCell;
}

bool RealTimeArbiter::hasMotionForPiece(std::uint32_t pieceId) const {
    for (const Motion& motion : active_) {
        if (motion.pieceId == pieceId) {
            return true;
        }
    }
    return false;
}

void RealTimeArbiter::startMotion(std::uint32_t pieceId, Position source,
                                  Position destination) {
    if (hasMotionForPiece(pieceId)) {
        throw std::logic_error(
            "RealTimeArbiter::startMotion: piece already has an active motion");
    }

    const std::shared_ptr<Piece> piece = board_.getPieceAt(source);
    if (!piece || piece->getId() != pieceId) {
        throw std::invalid_argument(
            "RealTimeArbiter::startMotion: no such piece at source");
    }

    const int durationMs = computeDurationMs(source, destination);
    piece->setState(State::Moving);
    active_.push_back(Motion{pieceId, source, destination, 0, durationMs});
}

bool RealTimeArbiter::resolveArrival(const Motion& motion) {
    const std::shared_ptr<Piece> mover = board_.getPieceAt(motion.source);

    bool kingCaptured = false;
    const std::shared_ptr<Piece> target = board_.getPieceAt(motion.destination);
    if (target) {
        target->setState(State::Captured);
        kingCaptured = target->getKind() == Kind::King;
        board_.removePiece(motion.destination);
    }

    // movePiece clears the source cell, places the mover at the destination, and
    // syncs the piece's own cell (Board is the single source of truth).
    board_.movePiece(motion.source, motion.destination);
    mover->setState(State::Idle);
    return kingCaptured;
}

ArbiterResult RealTimeArbiter::advanceTime(int ms) {
    ArbiterResult result;

    for (Motion& motion : active_) {
        motion.elapsedMs += ms;
    }

    for (auto it = active_.begin(); it != active_.end();) {
        if (it->elapsedMs >= it->durationMs) {
            if (resolveArrival(*it)) {
                result.kingCaptured = true;
            }
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
    return result;
}

bool RealTimeArbiter::hasActiveMotion() const {
    return !active_.empty();
}

}  // namespace kfc::realtime
