#pragma once

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "client/input/include/game_access.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/realtime/include/motion.hpp"
#include "shared/logic/rules/include/rule_engine.hpp"

namespace kfc::net {

// The client's whole knowledge of the game. It never runs the rules of time: it
// keeps a replica board and the last frame of motions/cooldowns, rebuilt from
// the state the server broadcasts, and it publishes the events the server relays
// onto a local bus so this client's own MoveLog and ScoreBoard record them
// unchanged. It sends commands but never decides their fate -- the answer comes
// back as the next state.
//
// It is colour-agnostic on the way in (state is the same for everyone) and
// colour-aware on the way out: a command carries the colour of the piece being
// moved, read from the replica. Who is allowed to issue that colour is settled
// downstream -- by the sink's wiring in loopback (a seat per colour) and by the
// server's ownership check either way.
class RemoteGame : public input::GameAccess {
public:
    // Sends one command message on behalf of the given colour. The colour lets a
    // loopback wiring pick the right seat; a single-player socket ignores it and
    // lets the server's ownership check stand.
    using CommandSink = std::function<void(model::Color mover,
                                           const std::string& message)>;

    RemoteGame(model::Board replica, CommandSink sink);

    // Apply one message the server sent: a state frame refreshes the replica; a
    // move or capture event is republished locally; anything else is ignored.
    void receive(const std::string& message);

    engine::GameSnapshot getSnapshot() const;
    std::set<model::Position> legalDestinationsFor(model::Position source) const;
    bus::EventBus& events() { return bus_; }

    std::optional<model::Piece> pieceAt(model::Position cell) const override;
    void requestMove(model::Position from, model::Position to) override;
    void requestJump(model::Position cell) override;

private:
    // Whether a piece on this cell is mid-move or resting right now, read from the
    // last frame -- the replica alone cannot say, since it carries no piece state.
    bool isBusy(model::Position cell) const;
    // Encode and send a command for the piece on `from` going to `to` (to == from
    // is a jump). Does nothing if no piece sits there to name the mover.
    void sendCommand(model::Position from, model::Position to);

    model::Board replica_;
    CommandSink sink_;
    bus::EventBus bus_;
    rules::RuleEngine ruleEngine_;
    bool isOver_ = false;
    std::vector<realtime::MotionState> motions_;
    std::vector<realtime::CooldownState> cooldowns_;
};

}  // namespace kfc::net
