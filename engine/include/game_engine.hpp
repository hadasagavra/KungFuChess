#pragma once

#include <optional>
#include <string>

#include "model/include/board.hpp"
#include "model/include/game_state.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/real_time_arbiter.hpp"
#include "rules/include/rule_engine.hpp"

namespace kfc::engine {

// The outcome of a move/jump request. reason is always present: "ok" when
// accepted, otherwise a stable token ("game_over", "motion_in_progress",
// "not_idle", "no_piece", or a forwarded RuleEngine reason).
struct MoveResult {
    bool isAccepted;
    std::string reason;
};

// A read-only view of the logical game state for the renderer and BoardPrinter.
// It delegates width/height/pieceAt to the live Board and carries the game-over
// flag. Lightweight view: use it within the lifetime of the engine's board.
class GameSnapshot {
public:
    GameSnapshot(const model::Board& board, bool isOver);

    int width() const { return board_.width(); }
    int height() const { return board_.height(); }
    bool isOver() const { return isOver_; }
    std::optional<model::Piece> pieceAt(model::Position cell) const;

private:
    const model::Board& board_;
    bool isOver_;
};

// Application-service coordinator and public command boundary. It holds the game
// state, validates moves via RuleEngine, and delegates timing/transit to
// RealTimeArbiter (reading its arrival reports to end the game on king capture
// and to promote pawns). It contains no movement rules, rendering, or parsing.
class GameEngine {
public:
    explicit GameEngine(model::Board& board);

    MoveResult requestMove(model::Position source, model::Position destination);
    MoveResult requestJump(model::Position cell);
    void wait(int ms);
    GameSnapshot getSnapshot() const;
    bool isGameOver() const;

private:
    model::Board& board_;
    model::GameState gameState_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
};

}  // namespace kfc::engine
