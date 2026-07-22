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

// The letter that stands for a colour ('w' / 'b'). Exposed for the same reason
// as kindLetter: a caller that needs only the colour letter (the ROLE message
// names a player by it) reads this one mapping instead of re-deriving it.
char colorLetter(model::Color color);

// The single source of truth for the board's text encoding, in both directions,
// so BoardPrinter and BoardParser never encode the piece letters independently.
bool isEmptyToken(const std::string& token);
std::optional<PieceCode> pieceFromToken(const std::string& token);
std::string encodeCell(const std::optional<model::Piece>& cell);

// The two-character token for a piece identity, e.g. "wK" -- the inverse of
// pieceFromToken. Exposed because a piece identity is written in more places
// than a board cell: the wire protocol names the mover of a command and the
// victim of a capture the same way, and must not assemble the token itself.
std::string encodePieceToken(model::Color color, model::Kind kind);

}  // namespace kfc::io
