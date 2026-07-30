#include "third_party/doctest/doctest.h"

#include "server/app/include/seat_table.hpp"

using kfc::model::Color;
using kfc::server::Occupant;
using kfc::server::SeatTable;

TEST_CASE("colours are handed out white, then black, then spectator") {
    SeatTable seats;
    CHECK(seats.assignColor() == Color::White);
    seats.seat(1, Color::White, "Alice", 1200);
    CHECK(seats.assignColor() == Color::Black);
    seats.seat(2, Color::Black, "Bob", 1200);
    CHECK_FALSE(seats.assignColor());  // both seats taken: a spectator
}

TEST_CASE("a client's colour is read back, and a spectator has none") {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1200);
    seats.seat(2, std::nullopt, "Cara", 1300);  // spectator
    CHECK(seats.colorOf(1) == Color::White);
    CHECK_FALSE(seats.colorOf(2));
    CHECK_FALSE(seats.colorOf(99));  // unknown client
}

TEST_CASE("the occupant of a colour carries name and rating together") {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1234);
    const std::optional<Occupant> white = seats.occupantOf(Color::White);
    REQUIRE(white);
    CHECK(white->username == "Alice");
    CHECK(white->rating == 1234);
    CHECK(seats.clientOn(Color::White) == 1);
    CHECK_FALSE(seats.occupantOf(Color::Black));  // empty seat
}

TEST_CASE("removing a client frees the seat") {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1200);
    seats.remove(1);
    CHECK_FALSE(seats.colorOf(1));
    CHECK(seats.assignColor() == Color::White);  // white is open again
}

TEST_CASE("a rating can be updated in place") {
    SeatTable seats;
    seats.seat(1, Color::White, "Alice", 1200);
    seats.setRating(1, 1216);
    REQUIRE(seats.occupantOf(Color::White));
    CHECK(seats.occupantOf(Color::White)->rating == 1216);
}
