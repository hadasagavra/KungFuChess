#include "third_party/doctest/doctest.h"

#include <map>
#include <string>

#include "server/app/include/rating_settler.hpp"
#include "server/app/include/seat_table.hpp"

using kfc::model::Color;
using kfc::server::RatingSettler;
using kfc::server::SeatTable;

namespace {

// A seat table with two players at equal ratings, the common starting point.
SeatTable twoPlayers() {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1200);
    seats.seat(2, Color::Black, "Bob", 1200);
    return seats;
}

}  // namespace

TEST_CASE("settling raises the winner and lowers the loser") {
    SeatTable seats = twoPlayers();
    std::map<std::string, int> persisted;
    int broadcasts = 0;
    RatingSettler settler{
        seats,
        [&](const std::string& name, int rating) { persisted[name] = rating; },
        [&]() { ++broadcasts; }};

    settler.settle(Color::Black);  // Black lost, so White won

    // Between equals the K-factor splits evenly: +16 / -16.
    CHECK(seats.occupantOf(Color::White)->rating == 1216);
    CHECK(seats.occupantOf(Color::Black)->rating == 1184);
    CHECK(persisted["Alice"] == 1216);  // persisted through the sink
    CHECK(persisted["Bob"] == 1184);
    CHECK(broadcasts == 1);  // asked its owner to rebroadcast once
}

TEST_CASE("settling does nothing unless both seats hold players") {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1200);  // no black player
    std::map<std::string, int> persisted;
    int broadcasts = 0;
    RatingSettler settler{
        seats,
        [&](const std::string& name, int rating) { persisted[name] = rating; },
        [&]() { ++broadcasts; }};

    settler.settle(Color::Black);

    CHECK(persisted.empty());
    CHECK(broadcasts == 0);
    CHECK(seats.occupantOf(Color::White)->rating == 1200);  // unchanged
}

TEST_CASE("an empty sink is fine (local play persists nothing)") {
    SeatTable seats = twoPlayers();
    int broadcasts = 0;
    RatingSettler settler{seats, {}, [&]() { ++broadcasts; }};

    settler.settle(Color::White);  // White lost, Black won

    CHECK(seats.occupantOf(Color::Black)->rating == 1216);
    CHECK(broadcasts == 1);
}
