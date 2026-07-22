#include "shared/logic/io/include/piece_codec.hpp"

#include <array>

namespace kfc::io {
namespace {

struct KindLetter {
    model::Kind kind;
    char letter;
};

// The one place that defines which letter represents which piece kind.
constexpr std::array<KindLetter, 6> kindLetters{{
    {model::Kind::King, 'K'},
    {model::Kind::Queen, 'Q'},
    {model::Kind::Rook, 'R'},
    {model::Kind::Bishop, 'B'},
    {model::Kind::Knight, 'N'},
    {model::Kind::Pawn, 'P'},
}};

std::optional<model::Kind> kindOf(char letter) {
    for (const KindLetter& entry : kindLetters) {
        if (entry.letter == letter) return entry.kind;
    }
    return std::nullopt;
}

std::optional<model::Color> colorOf(char c) {
    if (c == whiteChar) return model::Color::White;
    if (c == blackChar) return model::Color::Black;
    return std::nullopt;
}

char charOf(model::Color color) {
    return color == model::Color::White ? whiteChar : blackChar;
}

}  // namespace

char kindLetter(model::Kind kind) {
    for (const KindLetter& entry : kindLetters) {
        if (entry.kind == kind) return entry.letter;
    }
    return '?';
}

bool isEmptyToken(const std::string& token) {
    return token.size() == 1 && token[0] == emptyToken;
}

std::optional<PieceCode> pieceFromToken(const std::string& token) {
    if (token.size() != 2) return std::nullopt;
    const std::optional<model::Color> color = colorOf(token[0]);
    const std::optional<model::Kind> kind = kindOf(token[1]);
    if (!color || !kind) return std::nullopt;
    return PieceCode{*color, *kind};
}

std::string encodePieceToken(model::Color color, model::Kind kind) {
    std::string token;
    token += charOf(color);
    token += kindLetter(kind);
    return token;
}

std::string encodeCell(const std::optional<model::Piece>& cell) {
    if (!cell) return std::string(1, emptyToken);
    return encodePieceToken(cell->getColor(), cell->getKind());
}

}  // namespace kfc::io
