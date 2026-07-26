#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/game_state.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/realtime/include/real_time_arbiter.hpp"
#include "shared/logic/rules/include/rule_engine.hpp"

namespace kfc::engine {

// The outcome of a move/jump request. reason is always present: MoveReason::Ok
// when accepted, otherwise the real-time reason the engine itself raised
// (GameOver, MotionInProgress, NotIdle, NoPiece) or a forwarded RuleEngine
// reason.
struct MoveResult {
    bool isAccepted;
    rules::MoveReason reason;
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

    // The bus the engine announces on, so listeners can subscribe to it. This
    // exposes a connection point, not the engine's state: what comes back is a
    // collaborator anyone may join, while the game's own data stays private.
    //
    // The engine publishes and never reads back, so it still learns nothing
    // about who is listening -- the moves log, the score, or a future network
    // publisher are added without touching this class. The bus lives as long as
    // the engine does, and a subscriber must outlive it.
    bus::EventBus& events() { return bus_; }

    MoveResult requestMove(model::Position source, model::Position destination);
    MoveResult requestJump(model::Position cell);
    void wait(int ms);
    GameSnapshot getSnapshot() const;
    bool isGameOver() const;

    // End the game now without a capture -- a forfeit. After this the game is
    // over exactly as if a king had been taken: no further command is accepted.
    // The engine owns the game-over state; who decides to forfeit (a disconnect,
    // say) is the caller's business.
    void endGame();

    // The squares the piece on `source` may currently move to, for display
    // highlighting. Empty unless the game is live and the piece is idle -- this
    // mirrors the real-time gate in requestMove, so the GUI only highlights
    // moves the engine would actually accept. The movement pattern comes from
    // the piece's own rule; no chess rules are defined here.
    std::set<model::Position> legalDestinationsFor(model::Position source) const;

private:
    // The real-time gate shared by requestMove and requestJump: may this piece
    // take a command right now? nullopt when it may, otherwise the exact reason.
    std::optional<rules::MoveReason> realTimeBlockFor(
        const std::shared_ptr<model::Piece>& piece) const;

    // Whether an enemy piece is standing on `destination` right now, so the
    // published event can say the move was ordered as a capture. Read at command
    // time: in a simultaneous game that occupant may still slip away.
    bool isCaptureTarget(model::Position destination, model::Color mover) const;

    // Build and publish the event for a command the engine has just accepted.
    void publishAccepted(const model::Piece& piece, model::Position from,
                         model::Position to, bool isJump);

    model::Board& board_;
    model::GameState gameState_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
    bus::EventBus bus_;
};

}  // namespace kfc::engine
