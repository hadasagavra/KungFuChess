#pragma once

#include <optional>
#include <string>

#include "server/app/include/rating_settler.hpp"
#include "server/app/include/seat_table.hpp"
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
// Who sits where lives in a SeatTable, not in loose parallel maps. The session is
// seated with an already-authenticated identity (name + rating): logging a player
// in and storing accounts belong to the lobby above it. When a result changes
// ratings, it reports each new value through a sink so the lobby can persist it;
// the session itself keeps no database.
//
// It speaks in typed messages: a decoded PlayerCommand arrives via applyCommand
// (the lobby already decoded it), and handleMessage is only a thin text ingress
// for the same-process loopback.
class GameSession {
public:
    // The persistence sink for settled ratings (defined with the RatingSettler
    // that owns the rating rule); aliased here for the callers that build a
    // session.
    using RatingSink = ::kfc::server::RatingSink;

    GameSession(model::Board& board, MessageTransport& transport,
                RatingSink onRatingChanged = {});

    // Seat a client under an authenticated identity: assign white, then black,
    // then spectator, tell it which, and publish the new roster.
    void addClient(ClientId client, const std::string& username, int rating);

    // The occupant record for a client if it is a player (white or black), or
    // nullopt for a spectator or an unknown client -- its color is therefore
    // always present. Used to reserve a seat over a disconnect.
    std::optional<Occupant> playerSeat(ClientId client) const;

    // Forget a client's seat entirely (a spectator left, or a reserved seat is
    // being handed to the same player on a new connection).
    void vacate(ClientId client);

    // Seat a returning player back into a specific colour (not the next free one),
    // tell it which, and publish the roster. Used to restore a reserved seat when
    // a disconnected player reconnects.
    void seatReturning(ClientId client, model::Color color,
                       const std::string& username, int rating);

    // Apply a decoded command from a seated client (authorization then engine).
    void applyCommand(ClientId client, const io::PlayerCommand& command);

    // A raw message arrived from a seated client. A thin ingress for loopback:
    // decode, and act only if it is a command. Malformed text is dropped.
    void handleMessage(ClientId client, const std::string& text);

    // End the game without a capture: `loser` forfeits (e.g. on disconnect). The
    // winner's rating rises and the loser's falls, exactly as a king capture.
    void forfeit(model::Color loser);

    // Advance the clock and broadcast the resulting frame to every client.
    void tick(int deltaMs);

    bool isOver() const;

private:
    // Tell a client which colour it was given.
    void sendRole(ClientId client, std::optional<model::Color> color);
    // Broadcast the current names and ratings so every client can display them.
    void broadcastRoster();

    model::Board& board_;
    engine::GameEngine engine_;
    MessageTransport& transport_;
    SeatTable seats_;
    RatingSettler settler_;
};

}  // namespace kfc::server
