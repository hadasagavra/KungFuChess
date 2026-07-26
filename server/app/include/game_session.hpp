#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "server/net/include/message_transport.hpp"
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
// It is seated with an already-authenticated identity (name + rating): logging a
// player in and storing accounts belong to the lobby above it, not to a single
// game. When a result changes ratings, it reports each new value through a sink
// so the lobby can persist it; the session itself keeps no database.
//
// It speaks in typed WireMessages, never in wire text: decoding and encoding
// happen at the io boundary, and the session routes messages with std::visit.
class GameSession {
public:
    // Told a player's new rating after a game, so the owner (the lobby) can
    // persist it. Empty when nothing needs persisting (e.g. local play).
    using RatingSink =
        std::function<void(const std::string& username, int newRating)>;

    GameSession(model::Board& board, MessageTransport& transport,
                RatingSink onRatingChanged = {});

    // A seated player's colour and identity, for the room's reconnection logic.
    struct Seat {
        model::Color color;
        std::string username;
        int rating;
    };

    // Seat a client under an authenticated identity: assign white, then black,
    // then spectator, tell it which, and publish the new roster.
    void addClient(ClientId client, const std::string& username, int rating);

    // The seat a client holds if it is a player (white or black), or nullopt for
    // a spectator or an unknown client. Used to reserve a seat over a disconnect.
    std::optional<Seat> playerSeat(ClientId client) const;

    // Forget a client's seat entirely (a spectator left, or a reserved seat is
    // being handed to the same player on a new connection).
    void vacate(ClientId client);

    // Seat a returning player back into a specific colour (not the next free one),
    // tell it which, and publish the roster. Used to restore a reserved seat when
    // a disconnected player reconnects.
    void seatReturning(ClientId client, model::Color color,
                       const std::string& username, int rating);

    // A message arrived from a seated client. Only a command means anything here;
    // everything else (a client's own login, the server's own answers) is not the
    // session's to act on. Malformed text is dropped -- expected on a network.
    void handleMessage(ClientId client, const std::string& text);

    // End the game without a capture: `loser` forfeits (e.g. on disconnect). The
    // winner's rating rises and the loser's falls, exactly as a king capture.
    void forfeit(model::Color loser);

    // Advance the clock and broadcast the resulting frame to every client.
    void tick(int deltaMs);

    bool isOver() const;

private:
    // The colour a client plays, or nullopt if it is a spectator or unknown.
    std::optional<model::Color> colorOf(ClientId client) const;
    // The colour the next client should get: white, then black, then none.
    std::optional<model::Color> assignColor() const;
    // Apply a command whose sender owns the colour it names.
    void handleCommand(ClientId client, const io::PlayerCommand& command);
    // The client seated on `color`, or end() if that seat is empty.
    std::map<ClientId, std::optional<model::Color>>::const_iterator clientOn(
        model::Color color) const;
    // The name of whoever sits on `color`, or nullopt if that seat is empty.
    std::optional<std::string> nameOfColor(model::Color color) const;
    // The rating of whoever sits on `color`, or nullopt when the name is.
    std::optional<int> ratingOfColor(model::Color color) const;
    // Broadcast the current names and ratings so every client can display them.
    void broadcastRoster();
    // Settle the game by Elo: raise the winner's rating, lower the loser's,
    // report both through the sink, and rebroadcast the roster. Does nothing
    // unless both seats hold players.
    void settleRatings(model::Color loser);

    model::Board& board_;
    engine::GameEngine engine_;
    MessageTransport& transport_;
    RatingSink onRatingChanged_;
    std::map<ClientId, std::optional<model::Color>> roles_;
    std::map<ClientId, std::string> names_;
    std::map<ClientId, int> ratings_;
};

}  // namespace kfc::server
