#include <doctest/doctest.h>

#include "input/include/board_mapper.hpp"
#include "model/include/position.hpp"

using kfc::input::BoardMapper;
using kfc::model::Position;

namespace {

constexpr int cellPx = 100;

// A board drawn at the frame origin: no panel to its left.
BoardMapper flushMapper() { return BoardMapper{8, 8, cellPx, 0, 0}; }

// The real app draws the board to the right of Black's panel, so clicks arrive
// offset by the panel's width.
constexpr int panelWidth = 220;
BoardMapper offsetMapper() {
    return BoardMapper{8, 8, cellPx, panelWidth, 0};
}

}  // namespace

TEST_CASE("a click inside a cell maps to that cell") {
    const BoardMapper mapper = flushMapper();

    CHECK(mapper.toCell(50, 50) == Position{0, 0});
    CHECK(mapper.toCell(150, 50) == Position{0, 1});
    CHECK(mapper.toCell(50, 150) == Position{1, 0});
}

TEST_CASE("the top-left of a cell maps to the same cell as its interior") {
    const BoardMapper mapper = flushMapper();

    CHECK(mapper.toCell(100, 100) == Position{1, 1});
    CHECK(mapper.toCell(199, 199) == Position{1, 1});
}

TEST_CASE("a click past the right or bottom edge is outside the board") {
    const BoardMapper mapper = flushMapper();

    CHECK_FALSE(mapper.toCell(800, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, 800).has_value());
}

TEST_CASE("a negative pixel is outside the board") {
    const BoardMapper mapper = flushMapper();

    CHECK_FALSE(mapper.toCell(-1, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, -1).has_value());
}

TEST_CASE("the last cell is still inside the board") {
    const BoardMapper mapper = flushMapper();

    CHECK(mapper.toCell(750, 750) == Position{7, 7});
}

TEST_CASE("an offset board maps clicks relative to its own origin") {
    const BoardMapper mapper = offsetMapper();

    // The same board-relative pixels as the flush case, shifted by the panel.
    CHECK(mapper.toCell(panelWidth + 50, 50) == Position{0, 0});
    CHECK(mapper.toCell(panelWidth + 150, 50) == Position{0, 1});
    CHECK(mapper.toCell(panelWidth + 750, 750) == Position{7, 7});
}

TEST_CASE("a click on the panel beside the board is outside it") {
    const BoardMapper mapper = offsetMapper();

    // Left of the board entirely: this is Black's move table, not a square.
    CHECK_FALSE(mapper.toCell(10, 50).has_value());
    CHECK_FALSE(mapper.toCell(panelWidth - 1, 50).has_value());
    // Past the board's right edge, where White's table begins.
    CHECK_FALSE(mapper.toCell(panelWidth + 800, 50).has_value());
}

TEST_CASE("a cell size other than the default is honoured") {
    const BoardMapper mapper{8, 8, 50, 0, 0};

    CHECK(mapper.toCell(25, 25) == Position{0, 0});
    CHECK(mapper.toCell(75, 25) == Position{0, 1});
    CHECK_FALSE(mapper.toCell(400, 25).has_value());
}
