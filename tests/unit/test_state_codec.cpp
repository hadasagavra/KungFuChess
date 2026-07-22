#include "third_party/doctest/doctest.h"

#include <vector>

#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/state_codec.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/realtime/include/motion.hpp"

using kfc::engine::GameSnapshot;
using kfc::io::buildBoard;
using kfc::io::decodeState;
using kfc::io::DecodedState;
using kfc::io::encodeState;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;
using kfc::realtime::CooldownState;
using kfc::realtime::MotionState;

namespace {

// A compact 3x3 board so the assertions can name every cell.
Board sampleBoard() {
    return buildBoard({"wK . .", ". bP .", ". . wR"});
}

}  // namespace

TEST_CASE("a frame's board round-trips into a replica") {
    Board board = sampleBoard();
    const GameSnapshot snapshot{board, false};

    Board replica{1, 1};
    const std::optional<DecodedState> decoded =
        decodeState(encodeState(snapshot), replica);
    REQUIRE(decoded);

    CHECK(replica.width() == 3);
    CHECK(replica.height() == 3);
    REQUIRE(replica.getPieceAt(Position{0, 0}));
    CHECK(replica.getPieceAt(Position{0, 0})->getKind() == Kind::King);
    CHECK(replica.getPieceAt(Position{0, 0})->getColor() == Color::White);
    REQUIRE(replica.getPieceAt(Position{1, 1}));
    CHECK(replica.getPieceAt(Position{1, 1})->getKind() == Kind::Pawn);
    CHECK(replica.getPieceAt(Position{1, 1})->getColor() == Color::Black);
    CHECK_FALSE(replica.isOccupied(Position{0, 1}));
}

TEST_CASE("the game-over flag round-trips") {
    Board board = sampleBoard();
    Board replica{1, 1};

    const GameSnapshot live{board, false};
    REQUIRE(decodeState(encodeState(live), replica));
    CHECK_FALSE(decodeState(encodeState(live), replica)->isOver);

    const GameSnapshot over{board, true};
    REQUIRE(decodeState(encodeState(over), replica));
    CHECK(decodeState(encodeState(over), replica)->isOver);
}

TEST_CASE("in-flight motions and cooldowns round-trip") {
    Board board = sampleBoard();
    const std::vector<MotionState> motions{{Position{2, 2}, Position{1, 2}, 0.5}};
    const std::vector<CooldownState> cooldowns{{Position{0, 0}, 0.25}};
    const GameSnapshot snapshot{board, false, motions, cooldowns};

    Board replica{1, 1};
    const std::optional<DecodedState> decoded =
        decodeState(encodeState(snapshot), replica);
    REQUIRE(decoded);

    REQUIRE(decoded->motions.size() == 1);
    CHECK(decoded->motions[0].from == Position{2, 2});
    CHECK(decoded->motions[0].to == Position{1, 2});
    CHECK(decoded->motions[0].progress == doctest::Approx(0.5));

    REQUIRE(decoded->cooldowns.size() == 1);
    CHECK(decoded->cooldowns[0].cell == Position{0, 0});
    CHECK(decoded->cooldowns[0].progress == doctest::Approx(0.25));
}

TEST_CASE("a frame with no motions or cooldowns decodes to empty lists") {
    Board board = sampleBoard();
    const GameSnapshot snapshot{board, false};

    Board replica{1, 1};
    const std::optional<DecodedState> decoded =
        decodeState(encodeState(snapshot), replica);
    REQUIRE(decoded);
    CHECK(decoded->motions.empty());
    CHECK(decoded->cooldowns.empty());
}

TEST_CASE("a malformed frame decodes to nothing and leaves the board untouched") {
    Board replica = sampleBoard();  // a real board we can prove is not overwritten

    // No over line: mandatory, so its absence is not a frame.
    CHECK_FALSE(decodeState("wK . .\n. bP .\n. . wR\n", replica));
    // Bad over flag.
    CHECK_FALSE(decodeState("over 2\nwK\n", replica));
    // Ragged board rows: the same failure board_parser reports for a file.
    CHECK_FALSE(decodeState("over 0\nwK .\n. . .\n", replica));
    // A motion line missing its progress field.
    CHECK_FALSE(decodeState("over 0\nwK\nmotion a1 a1\n", replica));

    // The board the caller passed in still stands.
    CHECK(replica.width() == 3);
    REQUIRE(replica.getPieceAt(Position{0, 0}));
    CHECK(replica.getPieceAt(Position{0, 0})->getKind() == Kind::King);
}
