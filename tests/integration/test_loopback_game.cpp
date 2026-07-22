#include "third_party/doctest/doctest.h"

#include "client/net/include/loopback_game.hpp"
#include "shared/logic/game_record/include/move_log.hpp"
#include "shared/logic/game_record/include/score_board.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::game_record::MoveLog;
using kfc::game_record::ScoreBoard;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;
using kfc::net::LoopbackGame;

namespace {

// A white knight on b1 and a black knight on b8, each with a legal opening: a
// full run through the protocol needs a move that both colours can make.
Board twoKnights() {
    return kfc::io::buildBoard({". bN . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". wN . . . . . ."});
}

const Position b1{7, 1};
const Position c3{5, 2};
const Position b8{0, 1};
const Position c6{2, 2};

// A knight leap crosses two cells' worth of travel time (2000 ms), so advance
// past that. Stepped in small frames the way the real loop ticks, rather than
// one big jump.
void advancePastArrival(LoopbackGame& game) {
    for (int elapsed = 0; elapsed < 2500; elapsed += 100) game.advance(100);
}

}  // namespace

TEST_CASE("the replica shows the starting position after the seed frame") {
    LoopbackGame game{twoKnights()};

    // The constructor seeds one frame, so the client's replica already holds the
    // real board -- decoded off a state message, not shared from the server.
    REQUIRE(game.pieceAt(b1));
    CHECK(game.pieceAt(b1)->getKind() == Kind::Knight);
    CHECK(game.pieceAt(b1)->getColor() == Color::White);
    CHECK_FALSE(game.pieceAt(c3));
}

TEST_CASE("a white move travels the whole protocol and lands on the replica") {
    LoopbackGame game{twoKnights()};

    game.requestMove(b1, c3);       // encoded, sent, authorized, run by the engine
    advancePastArrival(game);     // server clock advances; new state broadcast back

    CHECK(game.pieceAt(c3));
    CHECK_FALSE(game.pieceAt(b1));
}

TEST_CASE("both colours can be commanded from the one window") {
    LoopbackGame game{twoKnights()};

    game.requestMove(b1, c3);       // white seat
    game.requestMove(b8, c6);       // black seat
    advancePastArrival(game);

    CHECK(game.pieceAt(c3));
    CHECK(game.pieceAt(c6));
}

TEST_CASE("relayed events reach this client's own move log") {
    LoopbackGame game{twoKnights()};
    MoveLog moveLog;
    game.events().subscribe<kfc::model::MoveEvent>(
        [&moveLog](const kfc::model::MoveEvent& event) { moveLog.record(event); });

    game.requestMove(b1, c3);
    advancePastArrival(game);

    // The event was published by the server's engine, relayed over the loopback,
    // and republished on this client's local bus -- so its own log has it.
    REQUIRE(moveLog.entriesFor(Color::White).size() == 1);
    CHECK(moveLog.entriesFor(Color::White).front().from == b1);
    CHECK(moveLog.entriesFor(Color::White).front().to == c3);
}

TEST_CASE("a capture's cost reaches this client's own score") {
    // White knight b1 can reach c3; put a black pawn on c3 to be taken.
    Board board = kfc::io::buildBoard(
        {". . . . . . . .", ". . . . . . . .", ". . . . . . . .",
         ". . . . . . . .", ". . . . . . . .", ". . bP . . . . .",
         ". . . . . . . .", ". wN . . . . . ."});
    LoopbackGame game{std::move(board)};
    ScoreBoard scoreBoard;
    game.events().subscribe<kfc::model::CapturedPiece>(
        [&scoreBoard](const kfc::model::CapturedPiece& captured) {
            scoreBoard.record(captured);
        });

    game.requestMove(b1, c3);       // white knight takes the pawn on c3
    advancePastArrival(game);

    CHECK(scoreBoard.scoreFor(Color::White) > 0);
}
