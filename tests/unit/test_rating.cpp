#include "third_party/doctest/doctest.h"

#include "shared/logic/game_record/include/rating.hpp"

using kfc::game_record::defaultRating;
using kfc::game_record::expectedScore;
using kfc::game_record::updatedRating;

TEST_CASE("equal ratings expect an even game") {
    CHECK(expectedScore(1200, 1200) == doctest::Approx(0.5));
}

TEST_CASE("a higher rating expects to score more") {
    CHECK(expectedScore(1400, 1200) > 0.5);
    CHECK(expectedScore(1200, 1400) < 0.5);
    // The two sides of one game split a whole point.
    CHECK(expectedScore(1400, 1200) + expectedScore(1200, 1400) ==
          doctest::Approx(1.0));
}

TEST_CASE("a win raises the rating and a loss lowers it") {
    const int winner = updatedRating(1200, 1200, 1.0);
    const int loser = updatedRating(1200, 1200, 0.0);
    CHECK(winner > 1200);
    CHECK(loser < 1200);
    // Between equals the K-factor splits evenly: +16 / -16.
    CHECK(winner == 1216);
    CHECK(loser == 1184);
}

TEST_CASE("beating a stronger opponent gains more than beating a weaker one") {
    const int overUnderdog = updatedRating(1200, 1000, 1.0) - 1200;
    const int overFavourite = updatedRating(1200, 1400, 1.0) - 1200;
    CHECK(overFavourite > overUnderdog);
}

TEST_CASE("the default rating is the documented starting point") {
    CHECK(defaultRating == 1200);
}
