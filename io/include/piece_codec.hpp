#pragma once

#include <optional>
#include <string>

#include "model/include/piece.hpp"

namespace kfc::io {

inline constexpr char emptyToken = '.';
inline constexpr char whiteChar = 'w';
inline constexpr char blackChar = 'b';

// A decoded piece identity (color + kind) parsed from a two-character cell
// token such as "wK". Used by BoardParser to reconstruct pieces.
struct PieceCode {
    model::Color color;
    model::Kind kind;
};

// The single source of truth for the board's text encoding, in both directions,
// so BoardPrinter and BoardParser never encode the piece letters independently.
bool isEmptyToken(const std::string& token);
std::optional<PieceCode> pieceFromToken(const std::string& token);
std::string encodeCell(const std::optional<model::Piece>& cell);

}  // namespace kfc::io
