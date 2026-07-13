#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace kfc::io {



inline std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}
}