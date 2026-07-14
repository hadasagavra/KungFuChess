#pragma once

#include <optional>
#include <string>
#include <variant>

namespace kfc::texttests {

struct ClickCommand {
    int x;
    int y;
};

struct WaitCommand {
    int ms;
};

struct PrintBoardCommand {};

struct JumpCommand {
    int x;
    int y;
};


using Command = std::variant<ClickCommand, WaitCommand,JumpCommand,PrintBoardCommand>;

std::optional<Command> parseCommand(const std::string& line);

}