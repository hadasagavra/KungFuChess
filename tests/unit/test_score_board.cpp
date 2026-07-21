#include "third_party/doctest/doctest.h"

#include "model/include/game_event.hpp"
#include "model/include/piece.hpp"
#include "game_record/include/score_board.hpp"

using kfc::model::CapturedPiece;
using kfc::model::Color;
using kfc::model::costOf;
using kfc::model::Kind;
using kfc::model::opponentOf;
using kfc::game_record::ScoreBoard;

TEST_CASE("a new scoreboard has both players on zero") {
    const ScoreBoard board;

    CHECK(board.scoreFor(Color::White) == 0);
    CHECK(board.scoreFor(Color::Black) == 0);
}

TEST_CASE("capturing a piece credits the piece's opponent") {
    ScoreBoard board;

    board.onCapture(CapturedPiece{Kind::Queen, Color::Black});

    // A black queen was taken, so White gained its worth.
    CHECK(board.scoreFor(Color::White) == costOf(Kind::Queen));
    CHECK(board.scoreFor(Color::Black) == 0);
}

TEST_CASE("captures accumulate for each player independently") {
    ScoreBoard board;

    board.onCapture(CapturedPiece{Kind::Pawn, Color::Black});
    board.onCapture(CapturedPiece{Kind::Rook, Color::Black});
    board.onCapture(CapturedPiece{Kind::Knight, Color::White});

    CHECK(board.scoreFor(Color::White) ==
          costOf(Kind::Pawn) + costOf(Kind::Rook));
    CHECK(board.scoreFor(Color::Black) == costOf(Kind::Knight));
}

TEST_CASE("piece costs rank the pieces as the game values them") {
    CHECK(costOf(Kind::Pawn) < costOf(Kind::Knight));
    CHECK(costOf(Kind::Knight) == costOf(Kind::Bishop));
    CHECK(costOf(Kind::Bishop) < costOf(Kind::Rook));
    CHECK(costOf(Kind::Rook) < costOf(Kind::Queen));
    // Taking the king wins the game outright; it adds nothing to a total.
    CHECK(costOf(Kind::King) == 0);
}

TEST_CASE("a promoted piece is scored as what it became") {
    ScoreBoard board;

    // A pawn that promoted is captured as a queen, and is worth a queen.
    board.onCapture(CapturedPiece{Kind::Queen, Color::White});

    CHECK(board.scoreFor(Color::Black) == costOf(Kind::Queen));
}

TEST_CASE("opponentOf names the other side") {
    CHECK(opponentOf(Color::White) == Color::Black);
    CHECK(opponentOf(Color::Black) == Color::White);
}
