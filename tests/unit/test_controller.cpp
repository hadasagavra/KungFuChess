#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>

#include "engine/include/game_engine.hpp"
#include "input/include/board_mapper.hpp"
#include "input/include/controller.hpp"
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::engine::GameEngine;
using kfc::input::BoardMapper;
using kfc::input::Controller;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;

namespace {

// Pixel at the centre of a cell (row, col), given CELL_SIZE = 100.
int centre(int index) { return index * 100 + 50; }

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

}  // namespace

TEST_CASE("A first click maps pixels to the right cell and selects a piece") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{2, 3});  // row 2, col 3
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centre(3), centre(2));  // x from col, y from row

    REQUIRE(controller.selection().has_value());
    CHECK(*controller.selection() == Position{2, 3});
}

TEST_CASE("A first click on an empty cell is ignored") {
    Board board{8, 8};
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centre(1), centre(1));  // empty {1,1}

    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("A first click off the board is ignored when nothing is selected") {
    Board board{8, 8};
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(10000, 10000);  // far off board
    CHECK_FALSE(controller.selection().has_value());

    controller.handleClick(-5, -5);  // negative
    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("A second click inside the board sends the move and clears selection") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centre(0), centre(0));  // select {0,0}
    REQUIRE(controller.selection().has_value());

    controller.handleClick(centre(0), centre(1));  // second click -> {1,0}, legal rook move

    CHECK_FALSE(controller.selection().has_value());  // cleared

    // The move was actually forwarded: after enough time it arrives.
    engine.wait(1000);
    CHECK(engine.getSnapshot().pieceAt(Position{1, 0}).has_value());
    CHECK_FALSE(engine.getSnapshot().pieceAt(Position{0, 0}).has_value());
}

TEST_CASE("A second click off the board cancels the selection and sends nothing") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centre(0), centre(0));  // select {0,0}
    REQUIRE(controller.selection().has_value());

    controller.handleClick(10000, 10000);  // off board -> cancel

    CHECK_FALSE(controller.selection().has_value());

    // No move was sent: the piece never leaves its cell.
    engine.wait(2000);
    CHECK(engine.getSnapshot().pieceAt(Position{0, 0}).has_value());
}

TEST_CASE("Selection clears after a second click even when the move is illegal") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    GameEngine engine{board};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centre(0), centre(0));  // select {0,0}
    controller.handleClick(centre(1), centre(1));  // {1,1}: illegal diagonal for a rook

    CHECK_FALSE(controller.selection().has_value());  // still cleared

    // The engine rejected it, so nothing moved.
    engine.wait(1000);
    CHECK(engine.getSnapshot().pieceAt(Position{0, 0}).has_value());
}
