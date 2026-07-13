#include "io/include/board_parser.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "io/include/piece_codec.hpp"
#include "io/include/text.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::io {
namespace {

constexpr const char* boardMarker = "Board:";
constexpr const char* commandsMarker = "Commands:";
constexpr const char* rowWidthMismatch = "row_width_mismatch";
constexpr const char* unknownToken = "unknown_token";
constexpr const char* emptyBoard = "empty_board";

void advanceToBoardSection(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (trim(line) == boardMarker) return;
    }
}

std::vector<std::string> readGridRows(std::istream& in) {
    std::vector<std::string> rows;
    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim(line);
        if (trimmed == commandsMarker) break;
        if (!trimmed.empty()) rows.push_back(trimmed);
    }
    return rows;
}

std::vector<std::string> readCommandLines(std::istream& in) {
    std::vector<std::string> commands;
    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim(line);
        if (!trimmed.empty()) commands.push_back(trimmed);
    }
    return commands;
}

model::Board assembleBoard(const std::vector<std::string>& rows) {
    if (rows.empty()) {
        throw BoardParseError{emptyBoard};
    }

    std::vector<std::vector<std::string>> grid;
    grid.reserve(rows.size());
    for (const std::string& row : rows) {
        grid.push_back(tokenize(row));
    }

    const int height = static_cast<int>(grid.size());
    const int width = static_cast<int>(grid.front().size());
    if (width == 0) {
        throw BoardParseError{emptyBoard};
    }
    for (const std::vector<std::string>& cells : grid) {
        if (static_cast<int>(cells.size()) != width) {
            throw BoardParseError{rowWidthMismatch};
        }
    }

    model::Board board{width, height};
    std::uint32_t nextId = 1;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::string& token = grid[row][col];
            if (isEmptyToken(token)) {
                continue;
            }
            const std::optional<PieceCode> code = pieceFromToken(token);
            if (!code) {
                throw BoardParseError{unknownToken};
            }
            board.addPiece(std::make_shared<model::Piece>(
                nextId++, code->color, code->kind, model::Position{row, col}));
        }
    }
    return board;
}

}  // namespace

ParsedScript parseScript(std::istream& in) {
    advanceToBoardSection(in);
    const std::vector<std::string> rows = readGridRows(in);
    std::vector<std::string> commands = readCommandLines(in);
    return ParsedScript{assembleBoard(rows), std::move(commands)};
}

}  // namespace kfc::io
