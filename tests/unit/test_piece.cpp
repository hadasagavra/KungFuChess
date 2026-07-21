#include "third_party/doctest/doctest.h"

#include <cstdint>

#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::model::State;

TEST_CASE("Piece initializes all fields") {
    const Piece p{7, Color::White, Kind::Queen, Position{1, 2}, State::Idle};

    CHECK(p.getId() == 7u);
    CHECK(p.getColor() == Color::White);
    CHECK(p.getKind() == Kind::Queen);
    CHECK(p.getCell() == Position{1, 2});
    CHECK(p.getState() == State::Idle);
}

TEST_CASE("Piece state defaults to Idle when omitted") {
    const Piece p{1, Color::Black, Kind::Pawn, Position{6, 0}};
    CHECK(p.getState() == State::Idle);
}

TEST_CASE("Piece state can be updated Idle -> Moving -> Captured") {
    Piece p{7, Color::White, Kind::Queen, Position{1, 2}};
    REQUIRE(p.getState() == State::Idle);

    p.setState(State::Moving);
    CHECK(p.getState() == State::Moving);

    p.setState(State::Captured);
    CHECK(p.getState() == State::Captured);
}

TEST_CASE("Piece cell can be updated without affecting other fields") {
    Piece p{7, Color::White, Kind::Queen, Position{1, 2}};

    p.setCell(Position{5, 6});

    CHECK(p.getCell() == Position{5, 6});
    CHECK(p.getId() == 7u);
    CHECK(p.getColor() == Color::White);
    CHECK(p.getKind() == Kind::Queen);
}

TEST_CASE("Piece equality is identity-based (by id only)") {
    SUBCASE("same id but different color/kind/cell/state are equal") {
        const Piece a{1, Color::White, Kind::Queen, Position{0, 0}, State::Idle};
        const Piece b{1, Color::Black, Kind::Pawn, Position{7, 7}, State::Captured};

        CHECK(a == b);
        CHECK_FALSE(a != b);
    }
    SUBCASE("different id with identical attributes are not equal") {
        const Piece c{2, Color::White, Kind::Queen, Position{0, 0}, State::Idle};
        const Piece d{3, Color::White, Kind::Queen, Position{0, 0}, State::Idle};

        CHECK(c != d);
        CHECK_FALSE(c == d);
    }
}
