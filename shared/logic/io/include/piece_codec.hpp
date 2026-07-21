#pragma once

#include <optional>
#include <string>

#include "shared/logic/model/include/piece.hpp"

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

// The letter that stands for a piece kind ('N' for a knight, and so on). Exposed
// so that every textual rendering of a piece -- the board format and the move
// notation alike -- reads the same mapping instead of spelling out its own.
char kindLetter(model::Kind kind);

// The single source of truth for the board's text encoding, in both directions,
// so BoardPrinter and BoardParser never encode the piece letters independently.
bool isEmptyToken(const std::string& token);
std::optional<PieceCode> pieceFromToken(const std::string& token);
std::string encodeCell(const std::optional<model::Piece>& cell);

}  // namespace kfc::io
