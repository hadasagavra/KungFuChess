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
using kfc::model::State;
using kfc::realtime::CooldownState;
using kfc::realtime::MotionState;

namespace {

// A compact 3x3 board so the assertions can name every cell. buildBoard assigns
// ids in row-major order: wK -> 1, bP -> 2, wR -> 3.
Board sampleBoard() {
    return buildBoard({"wK . .", ". bP .", ". . wR"});
}

}  // namespace

TEST_CASE("a frame's pieces round-trip into a replica with id and identity") {
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
    // The id survives, so the view's Animator tracks the same piece across frames.
    CHECK(replica.getPieceAt(Position{0, 0})->getId() == 1);
    REQUIRE(replica.getPieceAt(Position{1, 1}));
    CHECK(replica.getPieceAt(Position{1, 1})->getKind() == Kind::Pawn);
    CHECK(replica.getPieceAt(Position{1, 1})->getId() == 2);
    CHECK_FALSE(replica.isOccupied(Position{0, 1}));
}

TEST_CASE("a piece's lifecycle state round-trips") {
    // Without this the replica would show every piece Idle, so the view could
    // never animate a slide, a jump, or a rest -- the regression this guards.
    Board board = sampleBoard();
    board.setPieceState(Position{0, 0}, State::Moving);
    board.setPieceState(Position{1, 1}, State::Airborne);
    board.setPieceState(Position{2, 2}, State::Resting);
    const GameSnapshot snapshot{board, false};

    Board replica{1, 1};
    const std::optional<DecodedState> decoded =
        decodeState(encodeState(snapshot), replica);
    REQUIRE(decoded);
    CHECK(replica.getPieceAt(Position{0, 0})->getState() == State::Moving);
    CHECK(replica.getPieceAt(Position{1, 1})->getState() == State::Airborne);
    CHECK(replica.getPieceAt(Position{2, 2})->getState() == State::Resting);
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

    // Both the over flag and the size line are mandatory.
    CHECK_FALSE(decodeState("size 3 3\npiece 1 wK a3 I\n", replica));
    CHECK_FALSE(decodeState("over 0\npiece 1 wK a3 I\n", replica));
    // Bad over flag.
    CHECK_FALSE(decodeState("over 2\nsize 3 3\n", replica));
    // Bad piece fields: unknown kind, unknown state letter, non-numeric id.
    CHECK_FALSE(decodeState("over 0\nsize 3 3\npiece 1 wZ a3 I\n", replica));
    CHECK_FALSE(decodeState("over 0\nsize 3 3\npiece 1 wK a3 X\n", replica));
    CHECK_FALSE(decodeState("over 0\nsize 3 3\npiece x wK a3 I\n", replica));
    // A piece off the declared board.
    CHECK_FALSE(decodeState("over 0\nsize 3 3\npiece 1 wK a9 I\n", replica));

    // The board the caller passed in still stands.
    CHECK(replica.width() == 3);
    REQUIRE(replica.getPieceAt(Position{0, 0}));
    CHECK(replica.getPieceAt(Position{0, 0})->getKind() == Kind::King);
}
