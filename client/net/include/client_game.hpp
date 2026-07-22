#pragma once

#include <set>

#include "client/input/include/game_access.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::net {

// The game as the frame loop drives it: a GameAccess (so the Controller can hold
// the same object) plus the four things the loop needs each frame -- advance the
// game, read its state to draw, ask where a selected piece may go, and reach the
// event stream the moves log and score subscribe to. One implementation runs the
// game over a same-process loopback; a socket-backed one will slot in beside it
// without the loop changing.
class ClientGame : public input::GameAccess {
public:
    virtual void advance(int deltaMs) = 0;
    virtual engine::GameSnapshot getSnapshot() const = 0;
    virtual std::set<model::Position> legalDestinationsFor(
        model::Position source) const = 0;
    virtual bus::EventBus& events() = 0;
};

}  // namespace kfc::net
