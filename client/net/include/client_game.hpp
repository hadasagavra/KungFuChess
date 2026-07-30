#pragma once

#include <optional>
#include <set>
#include <string>

#include "client/input/include/game_access.hpp"
#include "client/net/include/lobby_access.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::net {

// The game as the frame loop drives it. It is a GameAccess (so the board
// Controller can hold the same object) and a LobbyAccess (so the Home-screen
// controller can), plus the things the loop needs each frame -- advance the game,
// read its state to draw, ask where a selected piece may go, reach the event
// stream the moves log and score subscribe to, and read the seat/room state a
// game frame shows. One implementation runs the game over a same-process
// loopback; a socket-backed one slots in beside it without the loop changing.
class ClientGame : public input::GameAccess, public LobbyAccess {
public:
    virtual void advance(int deltaMs) = 0;
    virtual engine::GameSnapshot getSnapshot() const = 0;
    virtual std::set<model::Position> legalDestinationsFor(
        model::Position source) const = 0;
    virtual bus::EventBus& events() = 0;

    // The player names to show beside the board, empty until the server names
    // them (a local game leaves them empty and the caller supplies defaults).
    virtual std::string whiteName() const = 0;
    virtual std::string blackName() const = 0;
    // The player ratings to show, absent until the server sends them (a local
    // game has no accounts, so it leaves them absent).
    virtual std::optional<int> whiteRating() const = 0;
    virtual std::optional<int> blackRating() const = 0;
    // Why the server refused this client's login, if it did, so the composition
    // root can report it and stop. Absent on a local game and a successful login.
    virtual std::optional<std::string> authError() const = 0;

    // The lobby actions and isSearching()/statusMessage() come from LobbyAccess.
    // A local game has no lobby: it is always in a game, so those actions do
    // nothing and its state is empty. These game-frame reads stay here:
    // whether the server has seated this client, the room shown at the top of the
    // board, and a disconnected opponent's forfeit countdown.
    virtual bool isInGame() const = 0;
    virtual std::string roomId() const = 0;
    virtual std::optional<int> opponentCountdown() const = 0;
};

}  // namespace kfc::net
