#pragma once

#include <map>
#include <optional>
#include <string>

#include "server/net/include/message_transport.hpp"
#include "server/store/include/user_store.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::server {

// The authoritative host of one game. It owns the single GameEngine, gives each
// client a colour, turns the commands clients send into engine calls, and
// broadcasts what the engine reports. It is the Server layer and holds no game
// rules of its own: legality is the engine's answer, and the one judgment the
// session does make -- may THIS client move THAT colour -- is authorization, not
// a chess rule.
//
// It speaks in typed WireMessages, never in wire text: decoding and encoding
// happen at the io boundary, and the session routes messages with std::visit.
class GameSession {
public:
    GameSession(model::Board& board, MessageTransport& transport,
                UserStore& users);

    // Seat a client: assign white, then black, then spectator, and tell it which.
    void addClient(ClientId client);

    // A message arrived from a client. Only a command and a login mean anything
    // here; a client has no standing to send the server's own answers, so those
    // are ignored. Malformed text is dropped -- expected on a network, not an
    // error.
    void handleMessage(ClientId client, const std::string& text);

    // Advance the clock and broadcast the resulting frame to every client.
    void tick(int deltaMs);

private:
    // The colour a client plays, or nullopt if it is a spectator or unknown.
    std::optional<model::Color> colorOf(ClientId client) const;
    // The colour the next client should get: white, then black, then none.
    std::optional<model::Color> assignColor() const;
    // Apply a command whose sender owns the colour it names.
    void handleCommand(ClientId client, const io::PlayerCommand& command);
    // Authenticate a client's credentials against the store. On success record
    // its name and rating and tell everyone the new roster; on failure tell just
    // that client its login was refused.
    void handleLogin(ClientId client, const io::Login& login);
    // The client seated on `color`, or end() if that seat is empty.
    std::map<ClientId, std::optional<model::Color>>::const_iterator clientOn(
        model::Color color) const;
    // The name of whoever sits on `color`, or nullopt if that seat is empty or
    // its occupant has not logged in yet.
    std::optional<std::string> nameOfColor(model::Color color) const;
    // The rating of whoever sits on `color`, or nullopt when the name is.
    std::optional<int> ratingOfColor(model::Color color) const;
    // Broadcast the current names and ratings so every client can display them.
    void broadcastRoster();
    // The king was captured: settle the game by Elo. Raise the winner's rating
    // and lower the loser's, persist both, and rebroadcast the roster. Does
    // nothing unless both seats hold logged-in players.
    void settleRatings(model::Color loser);

    model::Board& board_;
    engine::GameEngine engine_;
    MessageTransport& transport_;
    UserStore& users_;
    std::map<ClientId, std::optional<model::Color>> roles_;
    std::map<ClientId, std::string> names_;
    std::map<ClientId, int> ratings_;
};

}  // namespace kfc::server
