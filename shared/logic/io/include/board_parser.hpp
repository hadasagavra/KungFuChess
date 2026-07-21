#pragma once

#include <istream>
#include <string>
#include <vector>

#include "shared/logic/model/include/board.hpp"

namespace kfc::io {

// Thrown when the input text cannot be parsed into a board. code is a stable,
// machine-readable token (e.g. "ROW_WIDTH_MISMATCH", "UNKNOWN_TOKEN").
struct ParseError {
    std::string code;
};

// The result of parsing a text input: the initial board plus the raw command
// lines to replay (each still to be interpreted by the script parser).
struct ParsedInput {
    model::Board board;
    std::vector<std::string> commands;
};

// Parse a "Board:" grid followed by a "Commands:" list from the stream.
ParsedInput parseInput(std::istream& in);

}  // namespace kfc::io
