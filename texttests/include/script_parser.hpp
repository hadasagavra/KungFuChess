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

using Command = std::variant<ClickCommand, WaitCommand, PrintBoardCommand>;

std::optional<Command> parseCommand(const std::string& line);

}