#include "engine/include/game_engine.hpp"

#include <memory>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/real_time_arbiter.hpp"
#include "rules/include/rule_engine.hpp"

namespace kfc::engine {

using model::Board;
using model::Piece;
using model::Position;

namespace {

constexpr const char* ReasonOk = "ok";
constexpr const char* ReasonGameOver = "game_over";
constexpr const char* ReasonMotionInProgress = "motion_in_progress";

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

MoveResult GameEngine::requestMove(std::uint32_t pieceId, Position source,
                                   Position destination) {
    if (gameState_.isOver()) {
        return {false, ReasonGameOver};
    }
    if (arbiter_.hasActiveMotion()) {
        return {false, ReasonMotionInProgress};
    }

    const rules::MoveValidation validation =
        ruleEngine_.validateMove(board_, source, destination);
    if (!validation.isValid) {
        return {false, validation.reason};
    }

    arbiter_.startMotion(pieceId, source, destination);
    return {true, ReasonOk};
}

void GameEngine::wait(int ms) {
    const realtime::ArbiterResult result = arbiter_.advanceTime(ms);
    if (result.kingCaptured) {
        gameState_.markOver();
    }
}

GameSnapshot GameEngine::getSnapshot() const {
    return GameSnapshot{board_, gameState_.isOver()};
}

bool GameEngine::isGameOver() const {
    return gameState_.isOver();
}

}  // namespace kfc::engine
