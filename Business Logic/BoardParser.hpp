#pragma once

#include <istream>
#include <string>
#include <vector>

#include "Board.hpp"

namespace kfc::logic {

struct ParseError {
    std::string code;
};

class BoardParser {
public:
    Board parse(std::istream& in, std::vector<std::string>& commands);

private:
    std::string trim(const std::string& line) const;
    std::vector<std::string> tokenize(const std::string& line) const;
    bool isValidToken(const std::string& token) const;
    void skipToBoardMarker(std::istream& in) const;
    std::vector<Row> readBoardRows(std::istream& in) const;
    void validateBoard(const std::vector<Row>& rows) const;
    std::vector<std::string> readCommands(std::istream& in) const;
};

}  // namespace kfc::logic