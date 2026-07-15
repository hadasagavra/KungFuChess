#include "third_party/doctest/doctest.h"

#include <optional>
#include <variant>

#include "texttests/include/script_parser.hpp"

using kfc::texttests::ClickCommand;
using kfc::texttests::Command;
using kfc::texttests::JumpCommand;
using kfc::texttests::parseCommand;
using kfc::texttests::PrintBoardCommand;
using kfc::texttests::WaitCommand;

TEST_CASE("parseCommand parses a click command") {
    const std::optional<Command> cmd = parseCommand("click 150 250");

    REQUIRE(cmd.has_value());
    REQUIRE(std::holds_alternative<ClickCommand>(*cmd));
    CHECK(std::get<ClickCommand>(*cmd).x == 150);
    CHECK(std::get<ClickCommand>(*cmd).y == 250);
}

TEST_CASE("parseCommand parses a jump command") {
    const std::optional<Command> cmd = parseCommand("jump 150 250");

    REQUIRE(cmd.has_value());
    REQUIRE(std::holds_alternative<JumpCommand>(*cmd));
    CHECK(std::get<JumpCommand>(*cmd).x == 150);
    CHECK(std::get<JumpCommand>(*cmd).y == 250);
}

TEST_CASE("parseCommand parses a wait command") {
    const std::optional<Command> cmd = parseCommand("wait 1000");

    REQUIRE(cmd.has_value());
    REQUIRE(std::holds_alternative<WaitCommand>(*cmd));
    CHECK(std::get<WaitCommand>(*cmd).ms == 1000);
}

TEST_CASE("parseCommand parses a print board command") {
    const std::optional<Command> cmd = parseCommand("print board");

    REQUIRE(cmd.has_value());
    CHECK(std::holds_alternative<PrintBoardCommand>(*cmd));
}

TEST_CASE("parseCommand tolerates surrounding and repeated whitespace") {
    const std::optional<Command> cmd = parseCommand("   click   10   20   ");

    REQUIRE(cmd.has_value());
    REQUIRE(std::holds_alternative<ClickCommand>(*cmd));
    CHECK(std::get<ClickCommand>(*cmd).x == 10);
    CHECK(std::get<ClickCommand>(*cmd).y == 20);
}

TEST_CASE("parseCommand rejects malformed input") {
    SUBCASE("empty line") {
        CHECK_FALSE(parseCommand("").has_value());
    }
    SUBCASE("blank line") {
        CHECK_FALSE(parseCommand("    ").has_value());
    }
    SUBCASE("unknown command") {
        CHECK_FALSE(parseCommand("teleport 1 2").has_value());
    }
    SUBCASE("jump with too few arguments") {
        CHECK_FALSE(parseCommand("jump 1").has_value());
    }
    SUBCASE("click with too few arguments") {
        CHECK_FALSE(parseCommand("click 1").has_value());
    }
    SUBCASE("click with too many arguments") {
        CHECK_FALSE(parseCommand("click 1 2 3").has_value());
    }
    SUBCASE("click with non-numeric arguments") {
        CHECK_FALSE(parseCommand("click a b").has_value());
    }
    SUBCASE("wait without an argument") {
        CHECK_FALSE(parseCommand("wait").has_value());
    }
    SUBCASE("print of something other than board") {
        CHECK_FALSE(parseCommand("print pieces").has_value());
    }
}
