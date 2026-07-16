#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

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
// flag and a frozen list of in-flight motions (so the display can interpolate a
// sliding piece's position). Lightweight view: use it within the lifetime of the
// engine's board.
class GameSnapshot {
public:
    GameSnapshot(const model::Board& board, bool isOver,
                 std::vector<realtime::MotionState> motions = {},
                 std::vector<realtime::CooldownState> cooldowns = {});

    int width() const { return board_.width(); }
    int height() const { return board_.height(); }
    bool isOver() const { return isOver_; }
    std::optional<model::Piece> pieceAt(model::Position cell) const;
    const std::vector<realtime::MotionState>& motions() const { return motions_; }
    const std::vector<realtime::CooldownState>& cooldowns() const {
        return cooldowns_;
    }

private:
    const model::Board& board_;
    bool isOver_;
    std::vector<realtime::MotionState> motions_;
    std::vector<realtime::CooldownState> cooldowns_;
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

    // The squares the piece on `source` may currently move to, for display
    // highlighting. Empty unless the game is live and the piece is idle -- this
    // mirrors the real-time gate in requestMove, so the GUI only highlights
    // moves the engine would actually accept. The movement pattern comes from
    // the piece's own rule; no chess rules are defined here.
    std::set<model::Position> legalDestinationsFor(model::Position source) const;

private:
    model::Board& board_;
    model::GameState gameState_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
};

}  // namespace kfc::engine
