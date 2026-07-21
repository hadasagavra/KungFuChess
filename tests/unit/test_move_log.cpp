#include "third_party/doctest/doctest.h"

#include "model/include/game_event.hpp"
#include "game_record/include/move_log.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::game_record::MoveLog;
using kfc::model::Position;

namespace {

MoveEvent moveBy(Color player, int timeMs) {
    return MoveEvent{timeMs,          player,        Kind::Pawn,
                     Position{6, 0},  Position{5, 0}, false, false};
}

}  // namespace

TEST_CASE("a new log is empty for both players") {
    const MoveLog log;

    CHECK(log.entriesFor(Color::White).empty());
    CHECK(log.entriesFor(Color::Black).empty());
}

TEST_CASE("a move is filed under the player who made it") {
    MoveLog log;

    log.onMove(moveBy(Color::White, 1000));

    CHECK(log.entriesFor(Color::White).size() == 1);
    CHECK(log.entriesFor(Color::Black).empty());
}

TEST_CASE("each player's moves are kept in the order they were made") {
    MoveLog log;

    log.onMove(moveBy(Color::White, 1000));
    log.onMove(moveBy(Color::Black, 1500));
    log.onMove(moveBy(Color::White, 2000));

    const auto& white = log.entriesFor(Color::White);
    REQUIRE(white.size() == 2);
    CHECK(white[0].timeMs == 1000);
    CHECK(white[1].timeMs == 2000);

    const auto& black = log.entriesFor(Color::Black);
    REQUIRE(black.size() == 1);
    CHECK(black[0].timeMs == 1500);
}

TEST_CASE("a logged entry keeps the whole move, not just its time") {
    MoveLog log;

    log.onMove(MoveEvent{4105, Color::White, Kind::Knight, Position{7, 1},
                         Position{5, 2}, true, false});

    const auto& entries = log.entriesFor(Color::White);
    REQUIRE(entries.size() == 1);
    CHECK(entries[0].kind == Kind::Knight);
    CHECK(entries[0].from == Position{7, 1});
    CHECK(entries[0].to == Position{5, 2});
    CHECK(entries[0].isCapture);
    CHECK_FALSE(entries[0].isJump);
}
