#include "third_party/doctest/doctest.h"

#include "shared/logic/io/include/command_notation.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::decodeCommand;
using kfc::io::encodeCommand;
using kfc::io::PlayerCommand;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;

namespace {

constexpr int boardHeight = 8;

// e2 and e5 on an 8-high board: file 'e' is column 4; rank 2 is row 6, rank 5
// is row 3 (row 0 is the top).
const Position e2{6, 4};
const Position e5{3, 4};

}  // namespace

TEST_CASE("a move command matches the slide format from the spec") {
    const PlayerCommand command{Color::White, Kind::Queen, e2, e5};
    CHECK(encodeCommand(command, boardHeight) == "WQe2e5");
}

TEST_CASE("a jump command writes the same square twice") {
    // from == to is the wire's way of saying a jump -- the domain already treats
    // a jump as a move onto its own square.
    const PlayerCommand jump{Color::White, Kind::Queen, e2, e2};
    CHECK(encodeCommand(jump, boardHeight) == "WQe2e2");
}

TEST_CASE("a decoded command round-trips back to the same text") {
    for (const std::string& text : {"WQe2e5", "bPa7a6", "WNb1c3", "WKe1e1"}) {
        const std::optional<PlayerCommand> command =
            decodeCommand(text, boardHeight);
        REQUIRE(command);
        CHECK(encodeCommand(*command, boardHeight) == text);
    }
}

TEST_CASE("decoding reads the colour and kind into the command") {
    const std::optional<PlayerCommand> command =
        decodeCommand("WQe2e5", boardHeight);
    REQUIRE(command);
    CHECK(command->player == Color::White);
    CHECK(command->kind == Kind::Queen);
    CHECK(command->from == e2);
    CHECK(command->to == e5);
}

TEST_CASE("the colour letter is case-folded against the shared codec") {
    // The wire writes the colour upper case ('W'), a board cell lower case ('w'),
    // but which letter means white is piece_codec's single answer -- so decoding
    // 'W' yields the same colour pieceFromToken gives for 'w'.
    const std::optional<PlayerCommand> command =
        decodeCommand("WPe2e4", boardHeight);
    REQUIRE(command);
    const std::optional<kfc::io::PieceCode> cell = kfc::io::pieceFromToken("wP");
    REQUIRE(cell);
    CHECK(command->player == cell->color);
}

TEST_CASE("a jump command decodes with from equal to to") {
    const std::optional<PlayerCommand> jump =
        decodeCommand("WQe2e2", boardHeight);
    REQUIRE(jump);
    CHECK(jump->from == jump->to);
}

TEST_CASE("malformed command text decodes to nothing") {
    // Bad input off a socket is expected, not exceptional: decode reports it by
    // returning nothing rather than throwing.
    CHECK_FALSE(decodeCommand("", boardHeight));
    CHECK_FALSE(decodeCommand("WQ", boardHeight));            // no squares
    CHECK_FALSE(decodeCommand("WQe2", boardHeight));          // one square
    CHECK_FALSE(decodeCommand("WQe2e5x", boardHeight));       // odd tail length
    CHECK_FALSE(decodeCommand("XQe2e5", boardHeight));        // unknown colour
    CHECK_FALSE(decodeCommand("WZe2e5", boardHeight));        // unknown kind
    CHECK_FALSE(decodeCommand("WQz2e5", boardHeight));        // bad source file
    CHECK_FALSE(decodeCommand("WQe9e5", boardHeight));        // rank off the board
}
