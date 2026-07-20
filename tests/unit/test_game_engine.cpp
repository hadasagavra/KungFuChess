#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <set>

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
using kfc::rules::MoveReason;

namespace {

// Move + jump durations and the cooldown are all 1000ms in the test constants.
constexpr int oneCellMs = 1000;
constexpr int cooldownMs = 1000;
constexpr int jumpMs = 1000;

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

    const MoveResult result = engine.requestMove(Position{4, 4}, Position{4, 5});

    CHECK(result.isAccepted);
    CHECK(result.reason == MoveReason::Ok);
}

TEST_CASE("requestMove rejects a second move while one is in progress") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::White, Kind::Rook, Position{0, 0});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 5}).isAccepted);

    const MoveResult result = engine.requestMove(Position{0, 0}, Position{0, 1});
    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == MoveReason::MotionInProgress);
}

TEST_CASE("requestMove forwards the RuleEngine rejection reason") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    SUBCASE("illegal geometry") {
        const MoveResult result = engine.requestMove(Position{4, 4}, Position{5, 5});
        CHECK_FALSE(result.isAccepted);
        CHECK(result.reason == MoveReason::IllegalPieceMove);
    }
    SUBCASE("empty source") {
        const MoveResult result = engine.requestMove(Position{2, 2}, Position{2, 3});
        CHECK_FALSE(result.isAccepted);
        CHECK(result.reason == MoveReason::EmptySource);
    }
}

TEST_CASE("wait moves the piece only on arrival") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};
    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 5}).isAccepted);

    SUBCASE("sub-threshold: piece still on source") {
        engine.wait(oneCellMs - 1);
        const GameSnapshot snap = engine.getSnapshot();
        CHECK(snap.pieceAt(Position{4, 4}).has_value());
        CHECK_FALSE(snap.pieceAt(Position{4, 5}).has_value());
    }
    SUBCASE("at threshold: piece arrives at destination") {
        engine.wait(oneCellMs);
        const GameSnapshot snap = engine.getSnapshot();
        CHECK_FALSE(snap.pieceAt(Position{4, 4}).has_value());
        CHECK(snap.pieceAt(Position{4, 5}).has_value());
        CHECK(rook->getCell() == Position{4, 5});
    }
}

TEST_CASE("a piece must cool down before it can move again") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 5}).isAccepted);
    engine.wait(oneCellMs);  // arrives -> resting (cooldown)

    const MoveResult duringCooldown = engine.requestMove(Position{4, 5}, Position{4, 6});
    CHECK_FALSE(duringCooldown.isAccepted);
    CHECK(duringCooldown.reason == MoveReason::NotIdle);

    engine.wait(cooldownMs);  // cooldown elapses -> idle
    CHECK(engine.requestMove(Position{4, 5}, Position{4, 6}).isAccepted);
}

TEST_CASE("a pawn that reaches the last row is promoted to a queen") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Pawn, Position{1, 4});  // one step from row 0
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{1, 4}, Position{0, 4}).isAccepted);
    engine.wait(oneCellMs);

    const std::optional<Piece> promoted = engine.getSnapshot().pieceAt(Position{0, 4});
    REQUIRE(promoted.has_value());
    CHECK(promoted->getKind() == Kind::Queen);
}

TEST_CASE("requestJump keeps the piece in place and enforces idleness") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Knight, Position{4, 4});
    GameEngine engine{board};

    SUBCASE("jumping an idle piece is accepted and it stays put") {
        CHECK(engine.requestJump(Position{4, 4}).isAccepted);
        engine.wait(jumpMs);  // lands
        CHECK(engine.getSnapshot().pieceAt(Position{4, 4}).has_value());
    }
    SUBCASE("cannot jump a piece that is not idle") {
        REQUIRE(engine.requestJump(Position{4, 4}).isAccepted);  // now airborne
        const MoveResult again = engine.requestJump(Position{4, 4});
        CHECK_FALSE(again.isAccepted);
        CHECK(again.reason == MoveReason::NotIdle);
    }
    SUBCASE("cannot jump an empty cell") {
        const MoveResult empty = engine.requestJump(Position{0, 0});
        CHECK_FALSE(empty.isAccepted);
        CHECK(empty.reason == MoveReason::NoPiece);
    }
}

TEST_CASE("capturing the king ends the game") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::King, Position{4, 6});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 6}).isAccepted);
    engine.wait(2 * oneCellMs);

    CHECK(engine.isGameOver());
    CHECK(engine.getSnapshot().isOver());

    const MoveResult afterOver = engine.requestMove(Position{4, 6}, Position{4, 7});
    CHECK_FALSE(afterOver.isAccepted);
    CHECK(afterOver.reason == MoveReason::GameOver);
}

TEST_CASE("a non-king capture does not end the game") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 6}).isAccepted);
    engine.wait(2 * oneCellMs);

    CHECK_FALSE(engine.isGameOver());
    engine.wait(cooldownMs);  // let the rook finish cooling down
    CHECK(engine.requestMove(Position{4, 6}, Position{4, 7}).isAccepted);
}

TEST_CASE("legalDestinationsFor returns the idle piece's rule destinations") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    const std::set<Position> dests = engine.legalDestinationsFor(Position{4, 4});

    // A lone rook slides its whole rank and file: 7 + 7 squares.
    CHECK(dests.size() == 14);
    CHECK(dests.count(Position{4, 5}) == 1);  // along the rank
    CHECK(dests.count(Position{0, 4}) == 1);  // along the file
    CHECK(dests.count(Position{4, 4}) == 0);  // never its own cell
    CHECK(dests.count(Position{5, 5}) == 0);  // not on a rook line
}

TEST_CASE("legalDestinationsFor is empty when no move can start") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    GameEngine engine{board};

    SUBCASE("an empty cell has no destinations") {
        CHECK(engine.legalDestinationsFor(Position{0, 0}).empty());
    }
    SUBCASE("a moving (non-idle) piece has no destinations") {
        REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 5}).isAccepted);
        CHECK(engine.legalDestinationsFor(Position{4, 4}).empty());
    }
}

TEST_CASE("legalDestinationsFor is empty once the game is over") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    place(board, 2, Color::Black, Kind::King, Position{4, 6});
    place(board, 3, Color::White, Kind::Knight, Position{0, 0});  // stays idle
    GameEngine engine{board};

    REQUIRE(engine.requestMove(Position{4, 4}, Position{4, 6}).isAccepted);
    engine.wait(2 * oneCellMs);  // rook captures the king -> game over
    REQUIRE(engine.isGameOver());

    // The idle knight would normally have moves; the game-over gate wins.
    CHECK(engine.legalDestinationsFor(Position{0, 0}).empty());
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

    CHECK_FALSE(snap.pieceAt(Position{1, 1}).has_value());
}
