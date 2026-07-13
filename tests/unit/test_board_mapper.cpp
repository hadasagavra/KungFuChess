#include <doctest/doctest.h>

#include "input/include/board_mapper.hpp"
#include "model/include/position.hpp"

using kfc::input::BoardMapper;
using kfc::model::Position;

TEST_CASE("a click inside a cell maps to that cell") {
    BoardMapper mapper{8, 8};

    CHECK(mapper.toCell(50, 50) == Position{0, 0});
    CHECK(mapper.toCell(150, 50) == Position{0, 1});
    CHECK(mapper.toCell(50, 150) == Position{1, 0});
}

TEST_CASE("the top-left of a cell maps to the same cell as its interior") {
    BoardMapper mapper{8, 8};

    CHECK(mapper.toCell(100, 100) == Position{1, 1});
    CHECK(mapper.toCell(199, 199) == Position{1, 1});
}

TEST_CASE("a click past the right or bottom edge is outside the board") {
    BoardMapper mapper{8, 8};

    CHECK_FALSE(mapper.toCell(800, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, 800).has_value());
}

TEST_CASE("a negative pixel is outside the board") {
    BoardMapper mapper{8, 8};

    CHECK_FALSE(mapper.toCell(-1, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, -1).has_value());
}

TEST_CASE("the last cell is still inside the board") {
    BoardMapper mapper{8, 8};

    CHECK(mapper.toCell(750, 750) == Position{7, 7});
}