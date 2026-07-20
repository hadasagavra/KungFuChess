#include "engine/include/game_engine.hpp"

#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/real_time_arbiter.hpp"
#include "rules/include/piece_rules.hpp"
#include "rules/include/rule_engine.hpp"

namespace kfc::engine {

using model::Board;
using model::Piece;
using model::Position;
using model::State;

GameSnapshot::GameSnapshot(const Board& board, bool isOver,
                           std::vector<realtime::MotionState> motions,
                           std::vector<realtime::CooldownState> cooldowns)
    : board_(board),
      isOver_(isOver),
      motions_(std::move(motions)),
      cooldowns_(std::move(cooldowns)) {}

std::optional<Piece> GameSnapshot::pieceAt(Position cell) const {
    const std::shared_ptr<Piece> piece = board_.getPieceAt(cell);
    if (!piece) {
        return std::nullopt;
    }
    return *piece;
}

GameEngine::GameEngine(Board& board) : board_(board), arbiter_(board_) {}

std::optional<rules::MoveReason> GameEngine::realTimeBlockFor(
    const std::shared_ptr<Piece>& piece) const {
    // A piece already under way may not be sent somewhere new -- it has to finish
    // travelling first. Other pieces are free to move meanwhile.
    if (piece->getState() == State::Moving) {
        return rules::MoveReason::MotionInProgress;
    }
    // Resting (cooling down), airborne, or captured: not ready for a command.
    if (piece->getState() != State::Idle) {
        return rules::MoveReason::NotIdle;
    }
    return std::nullopt;
}

MoveResult GameEngine::requestMove(Position source, Position destination) {
    if (gameState_.isOver()) {
        return {false, rules::MoveReason::GameOver};
    }

    const rules::MoveValidation validation =
        ruleEngine_.validateMove(board_, source, destination);
    if (!validation.isValid) {
        return {false, validation.reason};
    }

    const std::shared_ptr<Piece> piece = board_.getPieceAt(source);
    if (const std::optional<rules::MoveReason> blocked = realTimeBlockFor(piece)) {
        return {false, *blocked};
    }

    arbiter_.startMotion(source, destination);
    return {true, rules::MoveReason::Ok};
}

MoveResult GameEngine::requestJump(Position cell) {
    if (gameState_.isOver()) {
        return {false, rules::MoveReason::GameOver};
    }

    const std::shared_ptr<Piece> piece = board_.getPieceAt(cell);
    if (!piece) {
        return {false, rules::MoveReason::NoPiece};
    }
    if (const std::optional<rules::MoveReason> blocked = realTimeBlockFor(piece)) {
        return {false, *blocked};
    }

    arbiter_.startJump(cell);
    return {true, rules::MoveReason::Ok};
}

void GameEngine::wait(int ms) {
    const std::vector<realtime::ArrivalReport> reports = arbiter_.advance(ms);
    for (const realtime::ArrivalReport& report : reports) {
        if (report.kingCaptured) {
            gameState_.markOver();
            return;
        }
        if (!report.landed) {
            continue;
        }
        // Apply promotion to a pawn that just arrived on its promotion row.
        const std::shared_ptr<Piece> piece = board_.getPieceAt(report.destination);
        if (piece) {
            if (const std::optional<model::Kind> promoted =
                    rules::promotedKind(board_, *piece)) {
                piece->setKind(*promoted);
            }
        }
    }
}

std::set<Position> GameEngine::legalDestinationsFor(Position source) const {
    if (gameState_.isOver()) {
        return {};
    }
    // Only an idle piece in a live game can start a move, so only then does a
    // highlight represent a move the engine would accept. The pattern itself
    // comes from the piece's rule -- reused, not reimplemented.
    const std::shared_ptr<Piece> piece = board_.getPieceAt(source);
    if (!piece || piece->getState() != State::Idle) {
        return {};
    }
    return rules::ruleFor(piece->getKind()).legalDestinations(board_, *piece);
}

GameSnapshot GameEngine::getSnapshot() const {
    return GameSnapshot{board_, gameState_.isOver(), arbiter_.activeMotions(),
                        arbiter_.activeCooldowns()};
}

bool GameEngine::isGameOver() const { return gameState_.isOver(); }

}  // namespace kfc::engine
