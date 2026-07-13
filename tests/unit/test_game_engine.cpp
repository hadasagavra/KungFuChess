#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <optional>

#include "engine/include/game_engine.hpp"
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::engine::GameEngine;
using kfc::engine::GameSnapshot;
using kfc::engine::MoveResult;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;

namespace {

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

}  // namespace

TEST_CASE("requestMove accepts a legal move") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    const MoveResult result = engine.requestMove(1, Position{4, 4}, Position{4, 5});

    CHECK(result.isAccepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("requestMove rejects a second move while one is in progress") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::White, Kind::Rook, Position{0, 0});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(1, Position{4, 4}, Position{4, 5}).isAccepted);

    const MoveResult result = engine.requestMove(2, Position{0, 0}, Position{0, 1});
    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "motion_in_progress");
}

TEST_CASE("requestMove forwards the RuleEngine rejection reason") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    SUBCASE("illegal geometry") {
        const MoveResult result = engine.requestMove(1, Position{4, 4}, Position{5, 5});
        CHECK_FALSE(result.isAccepted);
        CHECK(result.reason == "illegal_piece_move");
    }
    SUBCASE("empty source") {
        const MoveResult result = engine.requestMove(9, Position{2, 2}, Position{2, 3});
        CHECK_FALSE(result.isAccepted);
        CHECK(result.reason == "empty_source");
    }
}

TEST_CASE("wait advances motion and only moves the piece on arrival") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};
    REQUIRE(engine.requestMove(1, Position{4, 4}, Position{4, 5}).isAccepted);

    SUBCASE("sub-threshold: piece still on source, motion still active") {
        engine.wait(999);
        const GameSnapshot snap = engine.getSnapshot();
        CHECK(snap.pieceAt(Position{4, 4}).has_value());
        CHECK_FALSE(snap.pieceAt(Position{4, 5}).has_value());
        CHECK(engine.requestMove(1, Position{4, 4}, Position{4, 5}).reason ==
              "motion_in_progress");
    }
    SUBCASE("at threshold: piece arrives and a new move is accepted") {
        engine.wait(1000);
        const GameSnapshot snap = engine.getSnapshot();
        CHECK_FALSE(snap.pieceAt(Position{4, 4}).has_value());
        CHECK(snap.pieceAt(Position{4, 5}).has_value());
        CHECK(rook->getCell() == Position{4, 5});
        CHECK(engine.requestMove(1, Position{4, 5}, Position{4, 6}).isAccepted);
    }
}

TEST_CASE("capturing the king ends the game") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::King, Position{4, 6});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(1, Position{4, 4}, Position{4, 6}).isAccepted);
    engine.wait(2000);

    CHECK(engine.isGameOver());
    CHECK(engine.getSnapshot().isOver());

    const MoveResult afterOver = engine.requestMove(1, Position{4, 6}, Position{4, 7});
    CHECK_FALSE(afterOver.isAccepted);
    CHECK(afterOver.reason == "game_over");
}

TEST_CASE("a non-king capture does not end the game") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(1, Position{4, 4}, Position{4, 6}).isAccepted);
    engine.wait(2000);

    CHECK_FALSE(engine.isGameOver());
    CHECK_FALSE(engine.getSnapshot().isOver());
    CHECK(engine.requestMove(1, Position{4, 6}, Position{4, 7}).isAccepted);
}

TEST_CASE("getSnapshot reflects the current logical state") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::King, Position{0, 0});
    GameEngine engine{board};

    const GameSnapshot snap = engine.getSnapshot();

    CHECK(snap.width() == 8);
    CHECK(snap.height() == 8);
    CHECK_FALSE(snap.isOver());

    const std::optional<Piece> rook = snap.pieceAt(Position{4, 4});
    REQUIRE(rook.has_value());
    CHECK(rook->getColor() == Color::White);
    CHECK(rook->getKind() == Kind::Rook);

    const std::optional<Piece> king = snap.pieceAt(Position{0, 0});
    REQUIRE(king.has_value());
    CHECK(king->getColor() == Color::Black);
    CHECK(king->getKind() == Kind::King);

    CHECK_FALSE(snap.pieceAt(Position{1, 1}).has_value());  // empty cell
}
