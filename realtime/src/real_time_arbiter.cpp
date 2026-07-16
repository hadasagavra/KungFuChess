#include "realtime/include/real_time_arbiter.hpp"

#include <memory>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"

namespace kfc::realtime {

using model::Kind;
using model::Piece;
using model::Position;
using model::State;

RealTimeArbiter::RealTimeArbiter(model::Board& board) : board_(board) {}

std::vector<MotionState> RealTimeArbiter::activeMotions() const {
    std::vector<MotionState> motions;
    motions.reserve(active_.size());
    for (const Motion& motion : active_) {
        motions.push_back(motion.state());
    }
    return motions;
}

std::vector<CooldownState> RealTimeArbiter::activeCooldowns() const {
    std::vector<CooldownState> cooldowns;
    cooldowns.reserve(resting_.size());
    for (const Cooldown& cooldown : resting_) {
        cooldowns.push_back(cooldown.state());
    }
    return cooldowns;
}

bool RealTimeArbiter::startMotion(Position from, Position to) {
    if (hasActiveMotion()) {
        return false;
    }
    active_.emplace_back(from, to);
    board_.setPieceState(from, State::Moving);
    return true;
}

bool RealTimeArbiter::startJump(Position cell) {
    if (isAirborneAt(cell)) {
        return false;
    }
    airborne_.emplace_back(cell);
    board_.setPieceState(cell, State::Airborne);
    return true;
}

std::vector<ArrivalReport> RealTimeArbiter::advance(int deltaMs) {
    std::vector<ArrivalReport> reports;
    tickCooldowns(deltaMs);

    for (auto it = active_.begin(); it != active_.end();) {
        it->advance(deltaMs);
        if (it->hasArrived()) {
            reports.push_back(resolveArrival(*it));
            it = active_.erase(it);
        } else {
            ++it;
        }
    }

    landAirborne(deltaMs, reports);
    return reports;
}

ArrivalReport RealTimeArbiter::resolveArrival(const Motion& motion) {
    const Position from = motion.from();
    const Position to = motion.to();

    const std::shared_ptr<Piece> arriver = board_.getPieceAt(from);
    const std::shared_ptr<Piece> occupant = board_.getPieceAt(to);

    // An enemy motion arriving at an airborne piece's cell: the airborne piece
    // captures the arriver. Lift the airborne piece out; the arriver takes the
    // cell for now and is removed when the jump lands.
    if (occupant && occupant->getState() == State::Airborne && arriver &&
        arriver->getColor() != occupant->getColor()) {
        if (Jump* jump = jumpAt(to)) {
            jump->lift(occupant);
        }
        board_.removePiece(to);
        board_.movePiece(from, to);
        startCooldown(board_.getPieceAt(to)->getId(), to);
        return ArrivalReport{to, false, true};
    }

    bool kingCaptured = false;
    if (occupant) {
        kingCaptured = occupant->getKind() == Kind::King;
        occupant->setState(State::Captured);
        board_.removePiece(to);
    }
    board_.movePiece(from, to);
    startCooldown(board_.getPieceAt(to)->getId(), to);

    return ArrivalReport{to, kingCaptured, true};
}

void RealTimeArbiter::landAirborne(int deltaMs,
                                   std::vector<ArrivalReport>& reports) {
    for (auto it = airborne_.begin(); it != airborne_.end();) {
        it->advance(deltaMs);
        if (!it->hasLanded()) {
            ++it;
            continue;
        }

        const Position cell = it->cell();
        if (it->isLifted()) {
            // An enemy arrived during the jump: remove it, and the airborne piece
            // lands back in its cell (capturing the arriver).
            const std::shared_ptr<Piece> victim = board_.getPieceAt(cell);
            const bool kingCaptured = victim && victim->getKind() == Kind::King;
            if (victim) {
                victim->setState(State::Captured);
                board_.removePiece(cell);
            }
            std::shared_ptr<Piece> lander = it->lifted();
            lander->setCell(cell);
            board_.addPiece(lander);
            startCooldown(lander->getId(), cell);
            reports.push_back(ArrivalReport{cell, kingCaptured, false});
        } else {
            // No enemy arrived: the piece just lands and cools down.
            startCooldown(board_.getPieceAt(cell)->getId(), cell);
        }
        it = airborne_.erase(it);
    }
}

void RealTimeArbiter::startCooldown(std::uint32_t pieceId, Position cell) {
    board_.setPieceState(cell, State::Resting);
    resting_.emplace_back(pieceId, cell);
}

void RealTimeArbiter::tickCooldowns(int deltaMs) {
    for (auto it = resting_.begin(); it != resting_.end();) {
        it->advance(deltaMs);
        if (!it->hasElapsed()) {
            ++it;
            continue;
        }
        const std::shared_ptr<Piece> piece = board_.getPieceAt(it->cell());
        if (piece && piece->getId() == it->pieceId() &&
            piece->getState() == State::Resting) {
            piece->setState(State::Idle);
        }
        it = resting_.erase(it);
    }
}

bool RealTimeArbiter::isAirborneAt(Position cell) const {
    const std::shared_ptr<Piece> piece = board_.getPieceAt(cell);
    return piece && piece->getState() == State::Airborne;
}

Jump* RealTimeArbiter::jumpAt(Position cell) {
    for (Jump& jump : airborne_) {
        if (jump.cell() == cell) return &jump;
    }
    return nullptr;
}

}  // namespace kfc::realtime
