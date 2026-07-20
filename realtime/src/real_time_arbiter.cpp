#include "realtime/include/real_time_arbiter.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

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

bool RealTimeArbiter::hasMotionFor(Position cell) const {
    for (const Motion& motion : active_) {
        if (motion.currentCell() == cell) {
            return true;
        }
    }
    return false;
}

bool RealTimeArbiter::startMotion(Position from, Position to) {
    // Pieces travel in parallel; the only motion a new one may not join is
    // another motion of the same piece.
    if (hasMotionFor(from)) {
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
    advanceMotions(deltaMs, reports);
    landAirborne(deltaMs, reports);
    return reports;
}

void RealTimeArbiter::advanceMotions(int deltaMs,
                                     std::vector<ArrivalReport>& reports) {
    for (Motion& motion : active_) {
        motion.advance(deltaMs);
    }

    // One tick can be long enough to carry a piece across several cells, and
    // resolving one crossing can create another, so keep going until no motion
    // is mid-crossing.
    ResolvedFlags resolved(active_.size(), false);
    while (true) {
        const std::vector<std::size_t> crossings = crossingsByArrival(resolved);
        if (crossings.empty()) {
            break;
        }
        for (const std::size_t index : crossings) {
            // A crossing resolved earlier in this pass may have captured this
            // piece before it got to move.
            if (!resolved[index]) {
                resolveEntry(index, resolved, reports);
            }
        }
    }
    dropResolved(resolved);
}

std::vector<std::size_t> RealTimeArbiter::crossingsByArrival(
    const ResolvedFlags& resolved) const {
    std::vector<std::size_t> crossings;
    for (std::size_t index = 0; index < active_.size(); ++index) {
        if (!resolved[index] && active_[index].isEnteringNextCell()) {
            crossings.push_back(index);
        }
    }
    // The bigger the overshoot, the longer ago the piece crossed. Sorting by it
    // replays the crossings in the order they truly happened, which is what makes
    // "the piece arriving later takes the one already there" hold even when both
    // crossings land in the same tick. The sort is stable, so pieces that crossed
    // at the very same instant keep the order their motions began in.
    std::stable_sort(crossings.begin(), crossings.end(),
                     [this](std::size_t a, std::size_t b) {
                         return active_[a].arrivalOvershootMs() >
                                active_[b].arrivalOvershootMs();
                     });
    return crossings;
}

EntryOutcome RealTimeArbiter::classifyEntry(const Motion& motion) const {
    const std::shared_ptr<Piece> mover = board_.getPieceAt(motion.currentCell());
    const std::shared_ptr<Piece> occupant = board_.getPieceAt(motion.nextCell());

    if (!occupant) {
        return EntryOutcome::Free;
    }
    if (occupant->getColor() == mover->getColor()) {
        return EntryOutcome::BlockedByFriendly;
    }
    if (occupant->getState() == State::Airborne) {
        return EntryOutcome::CapturedByAirborne;
    }
    return EntryOutcome::Capture;
}

void RealTimeArbiter::resolveEntry(std::size_t index, ResolvedFlags& resolved,
                                   std::vector<ArrivalReport>& reports) {
    Motion& motion = active_[index];
    const EntryOutcome outcome = classifyEntry(motion);

    if (outcome == EntryOutcome::BlockedByFriendly) {
        // Two pieces of one colour never share a cell: the one that would have
        // walked in second stops on the cell it already holds.
        motion.stopHere();
        finishMotion(index, false, resolved, reports);
        return;
    }
    if (outcome == EntryOutcome::CapturedByAirborne) {
        yieldToAirborne(index, resolved, reports);
        return;
    }

    const bool kingCaptured = outcome == EntryOutcome::Capture &&
                              captureAt(motion.nextCell(), resolved);
    board_.movePiece(motion.currentCell(), motion.nextCell());
    motion.completeStep();

    if (motion.isComplete()) {
        finishMotion(index, kingCaptured, resolved, reports);
    } else if (kingCaptured) {
        // The game is already decided; the engine must not have to wait for the
        // rest of this journey to hear about it.
        reports.push_back(ArrivalReport{motion.currentCell(), true, true});
    }
}

bool RealTimeArbiter::captureAt(Position cell, ResolvedFlags& resolved) {
    const std::shared_ptr<Piece> victim = board_.getPieceAt(cell);
    const bool wasKing = victim->getKind() == Kind::King;
    victim->setState(State::Captured);
    board_.removePiece(cell);

    // A captured piece stops travelling, whether or not it was under way.
    for (std::size_t index = 0; index < active_.size(); ++index) {
        if (!resolved[index] && active_[index].currentCell() == cell) {
            resolved[index] = true;
        }
    }
    return wasKing;
}

void RealTimeArbiter::yieldToAirborne(std::size_t index, ResolvedFlags& resolved,
                                      std::vector<ArrivalReport>& reports) {
    Motion& motion = active_[index];
    const Position cell = motion.nextCell();

    // Lift the airborne piece out; the mover takes the cell for now and is
    // removed when the jump lands.
    const std::shared_ptr<Piece> jumper = board_.getPieceAt(cell);
    if (Jump* jump = jumpAt(cell)) {
        jump->lift(jumper);
    }
    board_.removePiece(cell);
    board_.movePiece(motion.currentCell(), cell);
    motion.completeStep();

    startCooldown(board_.getPieceAt(cell)->getId(), cell);
    reports.push_back(ArrivalReport{cell, false, true});
    resolved[index] = true;
}

void RealTimeArbiter::finishMotion(std::size_t index, bool kingCaptured,
                                   ResolvedFlags& resolved,
                                   std::vector<ArrivalReport>& reports) {
    const Position cell = active_[index].destination();
    startCooldown(board_.getPieceAt(cell)->getId(), cell);
    reports.push_back(ArrivalReport{cell, kingCaptured, true});
    resolved[index] = true;
}

void RealTimeArbiter::dropResolved(const ResolvedFlags& resolved) {
    std::size_t kept = 0;
    for (std::size_t index = 0; index < active_.size(); ++index) {
        if (resolved[index]) {
            continue;
        }
        if (kept != index) {
            active_[kept] = std::move(active_[index]);
        }
        ++kept;
    }
    active_.erase(active_.begin() + static_cast<std::ptrdiff_t>(kept),
                  active_.end());
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
