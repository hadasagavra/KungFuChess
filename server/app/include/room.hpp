#pragma once

#include <set>
#include <string>
#include <vector>

#include "server/app/include/game_session.hpp"
#include "server/app/include/room_transport.hpp"
#include "server/net/include/message_transport.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::server {

// A room's identifier, shown to players so they can join it.
using RoomId = std::string;

// One playable room: its own board and GameSession, scoped to its own members.
// A room is where a game actually happens; the lobby (RoomManager) creates rooms
// and routes clients into them, but never touches a game itself. Roles fall out
// of join order via the session's own seating rule -- the creator is white, the
// second joiner black, the rest spectators.
//
// It owns its board (so two rooms never share pieces) and a RoomTransport that
// keeps the session's broadcasts inside this room.
class Room {
public:
    Room(RoomId id, model::Board board, MessageTransport& underlying,
         GameSession::RatingSink onRatingChanged);

    const RoomId& id() const { return id_; }

    // Seat a client under an authenticated identity (name + rating). A brand-new
    // client is seated white/black/spectator by join order; a client whose
    // username matches a seat left vacant by a disconnect is instead reconnected
    // into that seat, cancelling its forfeit countdown.
    void addClient(ClientId client, const std::string& username, int rating);

    // Apply a decoded command from a seated client to the game.
    void applyCommand(ClientId client, const io::PlayerCommand& command);

    // A client's connection dropped. A seated player's seat is reserved and a
    // forfeit countdown begins; a spectator is simply removed.
    void handleDisconnect(ClientId client);

    // Advance this room's game clock, broadcast the frame, and run any forfeit
    // countdown (announcing the seconds left, forfeiting the game at zero).
    void tick(int deltaMs);

    bool hasMember(ClientId client) const { return members_.count(client) != 0; }
    bool empty() const { return members_.empty(); }

private:
    // A seat held open for a player who dropped, until they return or time runs
    // out. oldClient is the dead connection whose seat this was.
    struct Vacancy {
        model::Color color;
        std::string username;
        int rating;
        ClientId oldClient;
        int graceMsLeft;
        int lastSecondSent;
    };

    // Broadcast a message to this room's members.
    void broadcast(const io::WireMessage& message);

    RoomId id_;
    model::Board board_;
    std::set<ClientId> members_;
    RoomTransport transport_;
    GameSession session_;
    std::vector<Vacancy> vacancies_;
};

}  // namespace kfc::server
