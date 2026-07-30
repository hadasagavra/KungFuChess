#pragma once

#include <map>
#include <optional>
#include <string>

#include "server/net/include/message_transport.hpp"  // ClientId
#include "shared/logic/model/include/piece.hpp"       // model::Color

namespace kfc::server {

// One seat's occupant: the colour they play (nullopt for a spectator) and their
// authenticated identity.
struct Occupant {
    std::optional<model::Color> color;
    std::string username;
    int rating;
};

// Who sits where in one game: a single map from client to occupant, with the
// seating rule (white, then black, then spectator) and the lookups a session
// needs. It keeps one client's colour, name and rating together -- so they cannot
// drift out of sync as three parallel maps could -- and holds no transport, io,
// or game rule, so it is tested on its own.
class SeatTable {
public:
    // Seat (or re-seat) a client under a colour and identity.
    void seat(ClientId client, std::optional<model::Color> color,
              const std::string& username, int rating);
    // Remove a client entirely, freeing its seat.
    void remove(ClientId client);
    // Change a seated client's rating (after a game).
    void setRating(ClientId client, int rating);

    // The colour the next client should get: white, then black, then none.
    std::optional<model::Color> assignColor() const;
    // The colour a client plays, or nullopt if it is a spectator or unknown.
    std::optional<model::Color> colorOf(ClientId client) const;
    // The client seated on `color`, or nullopt if that seat is empty.
    std::optional<ClientId> clientOn(model::Color color) const;
    // The occupant seated on `color`, or nullopt if that seat is empty.
    std::optional<Occupant> occupantOf(model::Color color) const;

private:
    std::map<ClientId, Occupant> occupants_;
};

}  // namespace kfc::server
