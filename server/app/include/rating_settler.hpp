#pragma once

#include <functional>
#include <string>

#include "server/app/include/seat_table.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::server {

// Told a player's new rating after a game, so the owner (the lobby) can persist
// it. Empty when nothing needs persisting (e.g. local play).
using RatingSink =
    std::function<void(const std::string& username, int newRating)>;

// The rating rule as a collaborator: when a game ends -- a king captured or a
// forfeit -- it settles both players by Elo. It owns the scoring policy (who won,
// and how much a win is worth), so a GameSession need not change when that rule
// changes. It reads and updates the shared SeatTable, reports each new rating
// through the sink for persistence, and asks its owner to rebroadcast the roster.
class RatingSettler {
public:
    RatingSettler(SeatTable& seats, RatingSink sink,
                  std::function<void()> onRatingsChanged);

    // Settle a finished game whose `loser` colour lost: raise the winner's rating
    // and lower the loser's. A no-op unless both seats hold players.
    void settle(model::Color loser);

private:
    SeatTable& seats_;
    RatingSink sink_;
    std::function<void()> onRatingsChanged_;
};

}  // namespace kfc::server
