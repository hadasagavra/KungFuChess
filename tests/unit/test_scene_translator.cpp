#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>

#include "engine/include/game_engine.hpp"
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "view/include/scene_translator.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::model::State;
using kfc::view::buildSnapshot;
using kfc::view::GameSnapshot;
using kfc::view::PieceView;

namespace {

constexpr int cellPx = 100;

void place(Board& board, std::uint32_t id, Color color, Kind kind,
           Position cell) {
    board.addPiece(std::make_shared<Piece>(id, color, kind, cell));
}

}  // namespace

TEST_CASE("buildSnapshot carries the board dimensions") {
    Board board{8, 8};
    GameEngine engine{board};

    const GameSnapshot snapshot = buildSnapshot(engine.getSnapshot(), cellPx);

    CHECK(snapshot.boardWidth == 8);
    CHECK(snapshot.boardHeight == 8);
    CHECK(snapshot.pieces.empty());
}

TEST_CASE("buildSnapshot places a piece at its pixel position") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{1, 2});  // row 1, col 2
    GameEngine engine{board};

    const GameSnapshot snapshot = buildSnapshot(engine.getSnapshot(), cellPx);

    REQUIRE(snapshot.pieces.size() == 1);
    const PieceView& piece = snapshot.pieces.front();
    CHECK(piece.kind == Kind::Rook);
    CHECK(piece.color == Color::White);
    CHECK(piece.state == State::Idle);
    // x = col * cellPx, y = row * cellPx.
    CHECK(piece.position.x == 200);
    CHECK(piece.position.y == 100);
}

TEST_CASE("buildSnapshot slides a moving piece between its cells") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{1, 2});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{1, 2}, Position{1, 3}).isAccepted);
    engine.wait(500);  // halfway through the 1000ms one-cell slide

    const GameSnapshot snapshot = buildSnapshot(engine.getSnapshot(), cellPx);
    REQUIRE(snapshot.pieces.size() == 1);
    const PieceView& piece = snapshot.pieces.front();
    CHECK(piece.state == State::Moving);
    // Halfway between col 2 (x=200) and col 3 (x=300); row 1 unchanged.
    CHECK(piece.position.x == 250);
    CHECK(piece.position.y == 100);
}

TEST_CASE("buildSnapshot reports every occupied cell") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::King, Position{7, 4});
    place(board, 2, Color::Black, Kind::King, Position{0, 4});
    place(board, 3, Color::Black, Kind::Pawn, Position{1, 0});
    GameEngine engine{board};

    const GameSnapshot snapshot = buildSnapshot(engine.getSnapshot(), cellPx);

    CHECK(snapshot.pieces.size() == 3);
}
