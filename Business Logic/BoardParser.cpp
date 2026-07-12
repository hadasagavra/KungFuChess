#include "BoardParser.hpp"

#include <sstream>
#include <utility>

#include "PieceTypes.hpp"

namespace kfc::logic {

std::string BoardParser::trim(const std::string& line) const {
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

std::vector<std::string> BoardParser::tokenize(const std::string& line) const {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

bool BoardParser::isValidToken(const std::string& token) const {
    if (token == kEmptyCellToken) return true;
    if (token.size() != 2) return false;
    return isValidColor(token[0]) && charToPieceType(token[1]).has_value();
}

void BoardParser::skipToBoardMarker(std::istream& in) const {
    std::string line;
    while (std::getline(in, line)) {
        if (trim(line) == "Board:") return;
    }
}

std::vector<Row> BoardParser::readBoardRows(std::istream& in) const {
    std::vector<Row> rows;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed == "Commands:") break;
        if (trimmed.empty()) continue;
        rows.push_back(tokenize(trimmed));
    }
    return rows;
}

void BoardParser::validateBoard(const std::vector<Row>& rows) const {
    for (const auto& row : rows) {
        for (const auto& token : row) {
            if (!isValidToken(token)) {
                throw ParseError{"UNKNOWN_TOKEN"};
            }
        }
    }
    if (!rows.empty()) {
        size_t width = rows.front().size();
        for (const auto& row : rows) {
            if (row.size() != width) {
                throw ParseError{"ROW_WIDTH_MISMATCH"};
            }
        }
    }
}

std::vector<std::string> BoardParser::readCommands(std::istream& in) const {
    std::vector<std::string> commands;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) commands.push_back(trimmed);
    }
    return commands;
}

Board BoardParser::parse(std::istream& in, std::vector<std::string>& commands) {
    skipToBoardMarker(in);
    std::vector<Row> rows = readBoardRows(in);
    validateBoard(rows);
    commands = readCommands(in);
    return Board(std::move(rows));
}

}  // namespace kfc::logic