#include "shared/logic/game_record/include/rating.hpp"

#include <cmath>

namespace kfc::game_record {

namespace {
// The Elo logistic uses base 10 over a 400-point scale: a 400-point lead is a
// ten-to-one expectation.
constexpr double ratingScale = 400.0;
constexpr double logisticBase = 10.0;
}  // namespace

double expectedScore(int rating, int opponentRating) {
    const double exponent = (opponentRating - rating) / ratingScale;
    return 1.0 / (1.0 + std::pow(logisticBase, exponent));
}

int updatedRating(int rating, int opponentRating, double actualScore) {
    const double delta =
        kFactor * (actualScore - expectedScore(rating, opponentRating));
    return rating + static_cast<int>(std::lround(delta));
}

}  // namespace kfc::game_record
