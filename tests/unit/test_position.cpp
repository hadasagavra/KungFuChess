#include "third_party/doctest/doctest.h"

#include <set>
#include <sstream>
#include <string>

#include "shared/logic/model/include/position.hpp"

using kfc::model::Position;

TEST_CASE("Positions with the same row and col are equal") {
    const Position a{3, 4};
    const Position b{3, 4};

    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("Positions differing in row or col are not equal") {
    const Position base{3, 4};

    SUBCASE("different row") {
        CHECK(base != Position{5, 4});
        CHECK_FALSE(base == Position{5, 4});
    }
    SUBCASE("different col") {
        CHECK(base != Position{3, 9});
        CHECK_FALSE(base == Position{3, 9});
    }
    SUBCASE("different row and col") {
        CHECK(base != Position{5, 9});
    }
}

TEST_CASE("Position holds arbitrary ints and enforces no bounds") {
    // The value object has no notion of board size, so negative coordinates are
    // valid values and compare like any other.
    const Position a{-1, -2};
    const Position b{-1, -2};

    CHECK(a == b);
    CHECK(a != Position{-1, 2});
}

TEST_CASE("Position has a readable string representation") {
    std::ostringstream os;
    os << Position{2, 7};
    CHECK(os.str() == "Position(row=2, col=7)");
}

TEST_CASE("Position has a lexicographic ordering usable as a set key") {
    CHECK(Position{1, 2} < Position{1, 3});   // same row, smaller col
    CHECK(Position{1, 9} < Position{2, 0});   // smaller row wins over col
    CHECK_FALSE(Position{1, 2} < Position{1, 2});
    CHECK_FALSE(Position{2, 0} < Position{1, 9});

    std::set<Position> cells{{1, 1}, {0, 5}, {1, 1}};
    CHECK(cells.size() == 2);  // duplicate collapsed
}
