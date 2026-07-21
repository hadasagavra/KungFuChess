#include "third_party/doctest/doctest.h"

#include "shared/logic/io/include/move_notation.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::kindLetter;
using kfc::io::moveNotation;
using kfc::io::squareName;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::model::Position;

namespace {

constexpr int boardHeight = 8;

MoveEvent move(Kind kind, Position from, Position to, bool isCapture = false,
               bool isJump = false) {
    return MoveEvent{0, Color::White, kind, from, to, isCapture, isJump};
}

}  // namespace

TEST_CASE("a square is named by its file letter and its rank") {
    // Row 0 is the top of the board, which is rank 8 on an 8-high board.
    CHECK(squareName(Position{0, 0}, boardHeight) == "a8");
    CHECK(squareName(Position{7, 0}, boardHeight) == "a1");
    CHECK(squareName(Position{7, 7}, boardHeight) == "h1");
    CHECK(squareName(Position{4, 4}, boardHeight) == "e4");
}

TEST_CASE("a pawn move is written as the destination alone") {
    CHECK(moveNotation(move(Kind::Pawn, Position{6, 4}, Position{4, 4}),
                       boardHeight) == "e4");
}

TEST_CASE("a piece move carries the piece's letter") {
    CHECK(moveNotation(move(Kind::Knight, Position{7, 1}, Position{5, 2}),
                       boardHeight) == "Nc3");
    CHECK(moveNotation(move(Kind::Queen, Position{7, 3}, Position{3, 7}),
                       boardHeight) == "Qh5");
}

TEST_CASE("a capture is marked with an x") {
    CHECK(moveNotation(move(Kind::Bishop, Position{3, 1}, Position{5, 2}, true),
                       boardHeight) == "Bxc3");
}

TEST_CASE("a capturing pawn is identified by the file it came from") {
    // A pawn has no letter of its own, so its origin file names it: "exd4".
    CHECK(moveNotation(move(Kind::Pawn, Position{4, 4}, Position{4, 3}, true),
                       boardHeight) == "exd4");
}

TEST_CASE("a jump is marked and names the square jumped on") {
    CHECK(moveNotation(move(Kind::Knight, Position{5, 2}, Position{5, 2}, false,
                            true),
                       boardHeight) == "JNc3");
}

TEST_CASE("notation reads its letters from the shared piece codec") {
    // The notation must not carry a second, private letter table: asking the
    // codec directly has to give the same letter the notation used.
    const std::string knight =
        moveNotation(move(Kind::Knight, Position{7, 1}, Position{5, 2}),
                     boardHeight);
    CHECK(knight.front() == kindLetter(Kind::Knight));
}

TEST_CASE("ranks follow the board's own height") {
    // On a shorter board the same row is a different rank -- nothing here
    // assumes eight.
    CHECK(squareName(Position{0, 0}, 4) == "a4");
    CHECK(squareName(Position{3, 1}, 4) == "b1");
}
