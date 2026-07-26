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

// Build a Board from its already-split grid rows (each row a whitespace-split
// line of cell tokens), assigning piece ids in row-major order. Used by
// parseInput to read a board from a file, and by tests to state a board
// compactly. (A board off the wire is rebuilt by state_codec instead, which
// carries each piece's own id and lifecycle state rather than a bare grid.)
//
// Throws ParseError (empty_board, ROW_WIDTH_MISMATCH, UNKNOWN_TOKEN) on bad
// input, the same vocabulary parseInput reports.
model::Board buildBoard(const std::vector<std::string>& rows);

}  // namespace kfc::io
