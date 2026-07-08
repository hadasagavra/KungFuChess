
#include "Moverules.h"

#include <cstdlib>

namespace kfc::logic {

bool isMoveLegal(PieceType piece, char color, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    if (dr == 0 && dc == 0) return false;

    int adr = std::abs(dr);
    int adc = std::abs(dc);

    switch (piece) {
        case PieceType::King:   return adr <= 1 && adc <= 1;
        case PieceType::Rook:   return dr == 0 || dc == 0;
        case PieceType::Bishop: return adr == adc;
        case PieceType::Queen:  return dr == 0 || dc == 0 || adr == adc;
        case PieceType::Knight: return (adr == 1 && adc == 2) || (adr == 2 && adc == 1);
        case PieceType::Pawn: {
            int dir = (color == kWhiteColor) ? -1 : 1;
            if (dr != dir) return false;
            return adc <= 1;
        }
    }
    return false;
}

}  // namespace kfc::logic