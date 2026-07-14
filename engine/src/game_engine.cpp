#include "engine/include/game_engine.hpp"

#include <memory>
#include <optional>
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

namespace {

constexpr const char* reasonOk = "ok";
constexpr const char* reasonGameOver = "game_over";
constexpr const char* reasonMotionInProgress = "motion_in_progress";
constexpr const char* reasonNotIdle = "not_idle";
constexpr const char* reasonNoPiece = "no_piece";

}  // namespace

GameSnapshot::GameSnapshot(const Board& board, bool isOver)
    : board_(board), isOver_(isOver) {}

std::optional<Piece> GameSnapshot::pieceAt(Position cell) const {
    const std::shared_ptr<Piece> piece = board_.getPieceAt(cell);
    if (!piece) {
        return std::nullopt;
    }
    return *piece;
}

GameEngine::GameEngine(Board& board) : board_(board), arbiter_(board_) {}

MoveResult GameEngine::requestMove(Position source, Position destination) {
    if (gameState_.isOver()) {
        return {false, reasonGameOver};
    }
    if (arbiter_.hasActiveMotion()) {
        return {false, reasonMotionInProgress};
    }

    const rules::MoveValidation validation =
        ruleEngine_.validateMove(board_, source, destination);
    if (!validation.isValid) {
        return {false, validation.reason};
    }

    // The piece must be idle -- a resting (cooling-down) or airborne piece cannot
    // start a new move.
    const std::shared_ptr<Piece> piece = board_.getPieceAt(source);
    if (piece->getState() != State::Idle) {
        return {false, reasonNotIdle};
    }

    arbiter_.startMotion(source, destination);
    return {true, reasonOk};
}

MoveResult GameEngine::requestJump(Position cell) {
    if (gameState_.isOver()) {
        return {false, reasonGameOver};
    }

    const std::shared_ptr<Piece> piece = board_.getPieceAt(cell);
    if (!piece) {
        return {false, reasonNoPiece};
    }
    // A moving/resting/captured piece cannot jump.
    if (piece->getState() != State::Idle) {
        return {false, reasonNotIdle};
    }

    arbiter_.startJump(cell);
    return {true, reasonOk};
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

GameSnapshot GameEngine::getSnapshot() const {
    return GameSnapshot{board_, gameState_.isOver()};
}

bool GameEngine::isGameOver() const { return gameState_.isOver(); }

}  // namespace kfc::engine
