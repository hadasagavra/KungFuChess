#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "model/include/board.hpp"
#include "model/include/game_state.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/real_time_arbiter.hpp"
#include "rules/include/rule_engine.hpp"

namespace kfc::engine {

// The outcome of a move request. reason is always present: "ok" when accepted,
// otherwise a stable token ("game_over", "motion_in_progress", or a forwarded
// RuleEngine reason).
struct MoveResult {
    bool isAccepted;
    std::string reason;
};

// A read-only view of the logical game state for the renderer and BoardPrinter.
// It delegates width/height/pieceAt to the live Board (the single source of
// truth -- no duplicated dimensions) and carries the game-over flag. It is a
// lightweight view: use it within the lifetime of the engine's board.
class GameSnapshot {
public:
    GameSnapshot(const model::Board& board, bool isOver);

    int width() const { return board_.width(); }
    int height() const { return board_.height(); }
    bool isOver() const { return isOver_; }

    // The piece occupying a cell (a read-only copy), or std::nullopt if empty.
    std::optional<model::Piece> pieceAt(model::Position cell) const;

private:
    const model::Board& board_;
    bool isOver_;
};

// Application-service coordinator and public command boundary for the Controller
// and TextTestRunner. It holds the game state, validates moves via RuleEngine,
// and delegates all timing/transit to RealTimeArbiter, reading the arbiter's
// return value to learn about king captures (RealTimeArbiter never knows about
// GameEngine). It contains no movement rules, rendering, input, or parsing.
class GameEngine {
public:
    explicit GameEngine(model::Board& board);

    MoveResult requestMove(std::uint32_t pieceId, model::Position source,
                           model::Position destination);
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
