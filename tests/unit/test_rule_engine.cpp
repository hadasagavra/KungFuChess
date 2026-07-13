#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "rules/include/rule_engine.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::rules::MoveValidation;
using kfc::rules::RuleEngine;

namespace {

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

}  // namespace

TEST_CASE("validateMove rejects moves outside the board") {
    const RuleEngine engine;
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});

    SUBCASE("source out of bounds") {
        const MoveValidation r = engine.validateMove(board, Position{-1, 4}, Position{4, 4});
        CHECK_FALSE(r.isValid);
        CHECK(r.reason == "outside_board");
    }
    SUBCASE("destination out of bounds") {
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{8, 8});
        CHECK_FALSE(r.isValid);
        CHECK(r.reason == "outside_board");
    }
}

TEST_CASE("validateMove rejects an empty source cell") {
    const RuleEngine engine;
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});

    const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{4, 5});
    CHECK_FALSE(r.isValid);
    CHECK(r.reason == "empty_source");
}

TEST_CASE("validateMove rejects a friendly destination") {
    const RuleEngine engine;
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::White, Kind::Pawn, Position{4, 6});

    const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{4, 6});
    CHECK_FALSE(r.isValid);
    CHECK(r.reason == "friendly_destination");
}

TEST_CASE("validateMove rejects a geometrically illegal move") {
    const RuleEngine engine;

    SUBCASE("empty diagonal target for a rook") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{5, 5});
        CHECK_FALSE(r.isValid);
        CHECK(r.reason == "illegal_piece_move");
    }
    SUBCASE("enemy on an unreachable diagonal for a rook") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{5, 5});
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{5, 5});
        CHECK_FALSE(r.isValid);
        CHECK(r.reason == "illegal_piece_move");
    }
}

TEST_CASE("validateMove accepts legal moves and captures") {
    const RuleEngine engine;

    SUBCASE("rook slides to an empty square") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{4, 7});
        CHECK(r.isValid);
        CHECK(r.reason == "ok");
    }
    SUBCASE("rook captures an enemy inline") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{4, 6});
        CHECK(r.isValid);
        CHECK(r.reason == "ok");
    }
}

TEST_CASE("validateMove dispatches to the correct rule per Kind") {
    const RuleEngine engine;

    auto okMove = [&](Kind kind, Position from, Position to) {
        Board board{8, 8};
        place(board, 1, Color::White, kind, from);
        return engine.validateMove(board, from, to);
    };

    CHECK(okMove(Kind::Knight, Position{4, 4}, Position{6, 5}).reason == "ok");
    CHECK(okMove(Kind::Bishop, Position{4, 4}, Position{6, 6}).reason == "ok");
    CHECK(okMove(Kind::Queen, Position{4, 4}, Position{0, 0}).reason == "ok");
    CHECK(okMove(Kind::King, Position{4, 4}, Position{4, 5}).reason == "ok");
    CHECK(okMove(Kind::Pawn, Position{1, 4}, Position{0, 4}).reason == "ok");
}

TEST_CASE("validateMove applies its checks in the specified order") {
    const RuleEngine engine;

    SUBCASE("bounds is checked before empty source") {
        Board board{8, 8};
        const MoveValidation r = engine.validateMove(board, Position{9, 9}, Position{4, 4});
        CHECK(r.reason == "outside_board");
    }
    SUBCASE("empty source is checked before friendly destination") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Pawn, Position{0, 0});
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{0, 0});
        CHECK(r.reason == "empty_source");
    }
    SUBCASE("friendly destination is checked before piece rules") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{6, 6});  // unreachable diagonal
        const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{6, 6});
        CHECK(r.reason == "friendly_destination");
    }
}

TEST_CASE("validateMove never mutates the board") {
    const RuleEngine engine;
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    auto enemy = place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});

    const MoveValidation r = engine.validateMove(board, Position{4, 4}, Position{4, 6});
    REQUIRE(r.isValid);

    // The engine only inspects: both pieces remain exactly where they were.
    CHECK(board.getPieceAt(Position{4, 4}).get() == rook.get());
    CHECK(board.getPieceAt(Position{4, 6}).get() == enemy.get());
    CHECK(rook->getCell() == Position{4, 4});
}
