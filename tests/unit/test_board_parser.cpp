#include "third_party/doctest/doctest.h"

#include <sstream>
#include <string>

#include "io/include/board_parser.hpp"
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::io::ParsedInput;
using kfc::io::ParseError;
using kfc::io::parseInput;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;

namespace {

// Parse the input and return the ParseError code it throws, or "" on success.
std::string errorCode(const std::string& input) {
    std::istringstream in(input);
    try {
        parseInput(in);
    } catch (const ParseError& e) {
        return e.code;
    }
    return "";
}

}  // namespace

TEST_CASE("parseInput reads the board and the command lines") {
    std::istringstream in(
        "Board:\n"
        "wK . .\n"
        ". bR .\n"
        "Commands:\n"
        "click 50 50\n"
        "wait 1000\n");

    const ParsedInput parsed = parseInput(in);

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

TEST_CASE("parseInput skips text before Board: and trims lines") {
    std::istringstream in(
        "# a header line\n"
        "\n"
        "   Board:  \n"
        "  wP  \n"
        "Commands:\n");

    const ParsedInput parsed = parseInput(in);

    CHECK(parsed.board.width() == 1);
    CHECK(parsed.board.height() == 1);
    const auto pawn = parsed.board.getPieceAt(Position{0, 0});
    REQUIRE(pawn.get() != nullptr);
    CHECK(pawn->getKind() == Kind::Pawn);
    CHECK(parsed.commands.empty());
}

TEST_CASE("parseInput rejects rows of differing widths") {
    CHECK(errorCode(
              "Board:\n"
              "wK . .\n"
              ". bR\n"
              "Commands:\n") == "ROW_WIDTH_MISMATCH");
}

TEST_CASE("parseInput rejects an unknown piece token") {
    CHECK(errorCode(
              "Board:\n"
              "wZ . .\n"
              "Commands:\n") == "UNKNOWN_TOKEN");
}

TEST_CASE("parseInput rejects an empty board") {
    CHECK(errorCode(
              "Board:\n"
              "Commands:\n") == "empty_board");
}
