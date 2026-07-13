#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

#include "model/include/board.hpp"

namespace kfc::io {

// Thrown when the input text cannot be parsed into a board. The message is a
// stable, machine-readable code (e.g. "row_width_mismatch", "unknown_token",
// "empty_board"), consistent with the token style used elsewhere.
class BoardParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// The result of parsing a text input: the initial board plus the raw command
// lines to replay (each still to be interpreted by the script parser).
struct ParsedScript {
    model::Board board;
    std::vector<std::string> commands;
};

// Parse a "Board:" grid followed by a "Commands:" list from the stream.
ParsedScript parseScript(std::istream& in);

}  // namespace kfc::io
