#include "PieceTypes.h"

namespace kfc::logic {

bool isValidColor(char c) {
    return c == kWhiteColor || c == kBlackColor;
}

std::optional<PieceType> charToPieceType(char c) {
    switch (c) {
        case 'K': return PieceType::King;
        case 'Q': return PieceType::Queen;
        case 'R': return PieceType::Rook;
        case 'B': return PieceType::Bishop;
        case 'N': return PieceType::Knight;
        case 'P': return PieceType::Pawn;
        default:  return std::nullopt;
    }
}

}  // namespace kfc::logic