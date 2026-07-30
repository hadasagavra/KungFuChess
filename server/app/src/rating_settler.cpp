#include "server/app/include/rating_settler.hpp"

#include <optional>
#include <utility>

#include "shared/logic/game_record/include/rating.hpp"

namespace kfc::server {

RatingSettler::RatingSettler(SeatTable& seats, RatingSink sink,
                             std::function<void()> onRatingsChanged)
    : seats_(seats),
      sink_(std::move(sink)),
      onRatingsChanged_(std::move(onRatingsChanged)) {}

void RatingSettler::settle(model::Color loser) {
    const model::Color winner =
        loser == model::Color::White ? model::Color::Black : model::Color::White;

    // Both seats must hold players, or there is no pair to rate.
    const std::optional<Occupant> winnerSeat = seats_.occupantOf(winner);
    const std::optional<Occupant> loserSeat = seats_.occupantOf(loser);
    if (!winnerSeat || !loserSeat) return;

    const int newWinner =
        game_record::updatedRating(winnerSeat->rating, loserSeat->rating, 1.0);
    const int newLoser =
        game_record::updatedRating(loserSeat->rating, winnerSeat->rating, 0.0);

    if (sink_) {
        sink_(winnerSeat->username, newWinner);
        sink_(loserSeat->username, newLoser);
    }
    seats_.setRating(*seats_.clientOn(winner), newWinner);
    seats_.setRating(*seats_.clientOn(loser), newLoser);
    if (onRatingsChanged_) onRatingsChanged_();
}

}  // namespace kfc::server
