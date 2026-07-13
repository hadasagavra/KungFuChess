#include "third_party/doctest/doctest.h"

#include <sstream>

#include "io/include/board_parser.hpp"
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::io::BoardParseError;
using kfc::io::ParsedScript;
using kfc::io::parseScript;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;

TEST_CASE("parseScript reads the board and the command lines") {
    std::istringstream in(
        "Board:\n"
        "wK . .\n"
        ". bR .\n"
        "Commands:\n"
        "click 50 50\n"
        "wait 1000\n");

    const ParsedScript parsed = parseScript(in);

    CHECK(parsed.board.width() == 3);
    CHECK(parsed.board.height() == 2);

    const auto king = parsed.board.getPieceAt(Position{0, 0});
    REQUIRE(king.get() != nullptr);
    CHECK(king->getColor() == Color::White);
    CHECK(king->getKind() == Kind::King);

    const auto rook = parsed.board.getPieceAt(Position{1, 1});
    REQUIRE(rook.get() != nullptr);
    CHECK(rook->getColor() == Color::Black);
    CHECK(rook->getKind() == Kind::Rook);

    CHECK(parsed.board.getPieceAt(Position{0, 1}).get() == nullptr);  // empty cell

    REQUIRE(parsed.commands.size() == 2);
    CHECK(parsed.commands[0] == "click 50 50");
    CHECK(parsed.commands[1] == "wait 1000");
}

TEST_CASE("parseScript skips text before Board: and trims lines") {
    std::istringstream in(
        "# a header line\n"
        "\n"
        "   Board:  \n"
        "  wP  \n"
        "Commands:\n");

    const ParsedScript parsed = parseScript(in);

    CHECK(parsed.board.width() == 1);
    CHECK(parsed.board.height() == 1);
    const auto pawn = parsed.board.getPieceAt(Position{0, 0});
    REQUIRE(pawn.get() != nullptr);
    CHECK(pawn->getKind() == Kind::Pawn);
    CHECK(parsed.commands.empty());
}

TEST_CASE("parseScript rejects rows of differing widths") {
    std::istringstream in(
        "Board:\n"
        "wK . .\n"
        ". bR\n"
        "Commands:\n");

    CHECK_THROWS_WITH_AS(parseScript(in), "row_width_mismatch", BoardParseError);
}

TEST_CASE("parseScript rejects an unknown piece token") {
    std::istringstream in(
        "Board:\n"
        "wZ . .\n"
        "Commands:\n");

    CHECK_THROWS_WITH_AS(parseScript(in), "unknown_token", BoardParseError);
}

TEST_CASE("parseScript rejects an empty board") {
    std::istringstream in(
        "Board:\n"
        "Commands:\n");

    CHECK_THROWS_WITH_AS(parseScript(in), "empty_board", BoardParseError);
}
