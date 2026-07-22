#include "third_party/doctest/doctest.h"

#include "shared/logic/io/include/event_codec.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::decodeCapturedPiece;
using kfc::io::decodeMoveEvent;
using kfc::io::encodeCapturedPiece;
using kfc::io::encodeMoveEvent;
using kfc::model::CapturedPiece;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::model::Position;

namespace {

constexpr int boardHeight = 8;

// b1 and c3: file 'b' col 1 rank 1 (row 7), file 'c' col 2 rank 3 (row 5).
const Position b1{7, 1};
const Position c3{5, 2};

MoveEvent knightMove() {
    return MoveEvent{1500, Color::White, Kind::Knight, b1, c3, false, false};
}

bool sameEvent(const MoveEvent& a, const MoveEvent& b) {
    return a.timeMs == b.timeMs && a.player == b.player && a.kind == b.kind &&
           a.from == b.from && a.to == b.to && a.isCapture == b.isCapture &&
           a.isJump == b.isJump;
}

}  // namespace

TEST_CASE("a move event round-trips through the wire form unchanged") {
    const MoveEvent event = knightMove();
    const std::optional<MoveEvent> back =
        decodeMoveEvent(encodeMoveEvent(event, boardHeight), boardHeight);
    REQUIRE(back);
    CHECK(sameEvent(*back, event));
}

TEST_CASE("every field survives, timeMs included") {
    // A command drops the clock; an event must keep it, since the event is the
    // server's authoritative record of when the move was made.
    const MoveEvent capture{4105, Color::Black, Kind::Bishop,
                            Position{3, 1}, Position{5, 2}, true, false};
    const std::optional<MoveEvent> back =
        decodeMoveEvent(encodeMoveEvent(capture, boardHeight), boardHeight);
    REQUIRE(back);
    CHECK(back->timeMs == 4105);
    CHECK(back->player == Color::Black);
    CHECK(back->kind == Kind::Bishop);
    CHECK(back->isCapture);
}

TEST_CASE("a jump event keeps its jump flag and single square") {
    const MoveEvent jump{2000, Color::White, Kind::Queen,
                         Position{6, 4}, Position{6, 4}, false, true};
    const std::optional<MoveEvent> back =
        decodeMoveEvent(encodeMoveEvent(jump, boardHeight), boardHeight);
    REQUIRE(back);
    CHECK(back->isJump);
    CHECK(back->from == back->to);
}

TEST_CASE("a malformed move event decodes to nothing") {
    CHECK_FALSE(decodeMoveEvent("", boardHeight));
    CHECK_FALSE(decodeMoveEvent("wN b1 c3", boardHeight));         // too few fields
    CHECK_FALSE(decodeMoveEvent("wN b1 c3 1500 0 0 x", boardHeight)); // too many
    CHECK_FALSE(decodeMoveEvent("wZ b1 c3 1500 0 0", boardHeight)); // bad kind
    CHECK_FALSE(decodeMoveEvent("wN b9 c3 1500 0 0", boardHeight)); // rank off board
    CHECK_FALSE(decodeMoveEvent("wN b1 c3 abc 0 0", boardHeight));  // bad time
    CHECK_FALSE(decodeMoveEvent("wN b1 c3 1500 2 0", boardHeight)); // bad flag
}

TEST_CASE("a captured piece round-trips") {
    const CapturedPiece captured{Kind::Rook, Color::Black};
    const std::optional<CapturedPiece> back =
        decodeCapturedPiece(encodeCapturedPiece(captured));
    REQUIRE(back);
    CHECK(back->kind == Kind::Rook);
    CHECK(back->color == Color::Black);
}

TEST_CASE("a malformed captured piece decodes to nothing") {
    CHECK_FALSE(decodeCapturedPiece(""));
    CHECK_FALSE(decodeCapturedPiece("wP wP"));   // two tokens
    CHECK_FALSE(decodeCapturedPiece("wZ"));       // unknown kind
    CHECK_FALSE(decodeCapturedPiece("."));        // empty-cell token, not a piece
}
