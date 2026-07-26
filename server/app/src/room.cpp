#include "server/app/include/room.hpp"

#include <optional>
#include <utility>

#include "shared/logic/io/include/wire_message.hpp"

namespace kfc::server {
namespace {

// How long a dropped player's seat is held before they forfeit -- 20 seconds.
constexpr int disconnectGraceMs = 20000;

}  // namespace

Room::Room(RoomId id, model::Board board, MessageTransport& underlying,
           GameSession::RatingSink onRatingChanged)
    : id_(std::move(id)),
      board_(std::move(board)),
      transport_(underlying, members_),
      session_(board_, transport_, std::move(onRatingChanged)) {}

void Room::addClient(ClientId client, const std::string& username, int rating) {
    // A returning player reclaims the seat held open for their username, which
    // cancels the forfeit countdown; anyone else is seated fresh by join order.
    for (auto it = vacancies_.begin(); it != vacancies_.end(); ++it) {
        if (it->username == username) {
            session_.vacate(it->oldClient);
            session_.seatReturning(client, it->color, username, rating);
            members_.insert(client);
            vacancies_.erase(it);
            broadcast(io::WireMessage{io::OpponentReconnected{}});
            return;
        }
    }
    members_.insert(client);
    session_.addClient(client, username, rating);
}

void Room::handleMessage(ClientId client, const std::string& text) {
    session_.handleMessage(client, text);
}

void Room::handleDisconnect(ClientId client) {
    const std::optional<GameSession::Seat> seat = session_.playerSeat(client);
    members_.erase(client);
    if (!seat) {
        session_.vacate(client);  // a spectator (or unknown) simply leaves
        return;
    }
    if (session_.isOver()) return;  // nothing to forfeit once the game has ended
    // Keep the seat reserved (leave it in the session) so no one else takes the
    // colour, and start the countdown to a forfeit.
    vacancies_.push_back(
        Vacancy{seat->color, seat->username, seat->rating, client,
                disconnectGraceMs, -1});
}

void Room::tick(int deltaMs) {
    session_.tick(deltaMs);

    for (auto it = vacancies_.begin(); it != vacancies_.end();) {
        it->graceMsLeft -= deltaMs;
        if (it->graceMsLeft <= 0) {
            session_.forfeit(it->color);  // the absent player loses
            it = vacancies_.erase(it);
            continue;
        }
        // Announce the remaining whole seconds, but only when the number changes,
        // so the countdown is one message a second rather than one a frame.
        const int secondsLeft = (it->graceMsLeft + 999) / 1000;
        if (secondsLeft != it->lastSecondSent) {
            it->lastSecondSent = secondsLeft;
            broadcast(io::WireMessage{io::OpponentDisconnected{secondsLeft}});
        }
        ++it;
    }
}

void Room::broadcast(const io::WireMessage& message) {
    transport_.broadcast(io::encode(message, board_.height()));
}

}  // namespace kfc::server
