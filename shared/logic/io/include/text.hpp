#pragma once

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace kfc::io {

// A boolean on the wire is "1" or "0". The one place that maps between the two,
// so the codecs that carry flags (event_codec, state_codec) do not each spell
// out the same convention.
inline std::string encodeFlag(bool flag) { return flag ? "1" : "0"; }

inline std::optional<bool> decodeFlag(const std::string& token) {
    if (token == "1") return true;
    if (token == "0") return false;
    return std::nullopt;
}

// Remove leading and trailing whitespace.
inline std::string trim(const std::string& text) {
    static const char* whitespace = " \t\r\n";
    const std::size_t start = text.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    const std::size_t end = text.find_last_not_of(whitespace);
    return text.substr(start, end - start + 1);
}

// Split on runs of whitespace into non-empty tokens.
inline std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) tokens.push_back(token);
    return tokens;
}

}  // namespace kfc::io
