#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/board_printer.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::engine::GameSnapshot;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;

namespace {

void place(Board& board, std::uint32_t id, Color color, Kind kind, Position cell) {
    board.addPiece(std::make_shared<Piece>(id, color, kind, cell));
}

std::string print(const Board& board) {
    const GameSnapshot snapshot{board, false};
    std::ostringstream out;
    kfc::io::printBoard(snapshot, out);
    return out.str();
}

}  // namespace

TEST_CASE("An empty board prints as a grid of dots") {
    const Board board{3, 2};  // width 3, height 2
    CHECK(print(board) == ". . .\n. . .\n");
}

TEST_CASE("Pieces print as color+kind, empties as dots") {
    Board board{3, 2};
    place(board, 1, Color::White, Kind::King, Position{0, 0});
    place(board, 2, Color::Black, Kind::Rook, Position{1, 1});

    CHECK(print(board) == "wK . .\n. bR .\n");
}

TEST_CASE("Every kind maps to its letter") {
    Board board{6, 1};
    place(board, 1, Color::White, Kind::King, Position{0, 0});
    place(board, 2, Color::White, Kind::Queen, Position{0, 1});
    place(board, 3, Color::White, Kind::Rook, Position{0, 2});
    place(board, 4, Color::White, Kind::Bishop, Position{0, 3});
    place(board, 5, Color::White, Kind::Knight, Position{0, 4});
    place(board, 6, Color::White, Kind::Pawn, Position{0, 5});

    CHECK(print(board) == "wK wQ wR wB wN wP\n");
}

TEST_CASE("Both colors map to their letter") {
    Board board{2, 1};
    place(board, 1, Color::White, Kind::Pawn, Position{0, 0});
    place(board, 2, Color::Black, Kind::Pawn, Position{0, 1});

    CHECK(print(board) == "wP bP\n");
}
