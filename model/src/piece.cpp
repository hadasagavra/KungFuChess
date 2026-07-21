#include "model/include/piece.hpp"

#include <array>
#include <ostream>

namespace kfc::model {
namespace {

struct KindCost {
    Kind kind;
    int cost;
};

// The one place that defines what each piece kind is worth. A king is scored at
// zero because taking it ends the game outright -- the win is the reward, not a
// number added to a total.
constexpr int pawnCost = 1;
constexpr int knightCost = 3;
constexpr int bishopCost = 3;
constexpr int rookCost = 5;
constexpr int queenCost = 9;
constexpr int kingCost = 0;

constexpr std::array<KindCost, 6> kindCosts{{
    {Kind::King, kingCost},
    {Kind::Queen, queenCost},
    {Kind::Rook, rookCost},
    {Kind::Bishop, bishopCost},
    {Kind::Knight, knightCost},
    {Kind::Pawn, pawnCost},
}};

}  // namespace

Color opponentOf(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}

int costOf(Kind kind) {
    for (const KindCost& entry : kindCosts) {
        if (entry.kind == kind) return entry.cost;
    }
    return 0;
}

bool operator==(const Piece& a, const Piece& b) {
    return a.getId() == b.getId();
}

bool operator!=(const Piece& a, const Piece& b) {
    return !(a == b);
}

std::ostream& operator<<(std::ostream& os, Color c) {
    switch (c) {
        case Color::White: return os << "White";
        case Color::Black: return os << "Black";
    }
    return os << "Color(?)";
}

std::ostream& operator<<(std::ostream& os, Kind k) {
    switch (k) {
        case Kind::King:   return os << "King";
        case Kind::Queen:  return os << "Queen";
        case Kind::Rook:   return os << "Rook";
        case Kind::Bishop: return os << "Bishop";
        case Kind::Knight: return os << "Knight";
        case Kind::Pawn:   return os << "Pawn";
    }
    return os << "Kind(?)";
}

std::ostream& operator<<(std::ostream& os, State s) {
    switch (s) {
        case State::Idle:     return os << "Idle";
        case State::Moving:   return os << "Moving";
        case State::Airborne: return os << "Airborne";
        case State::Resting:  return os << "Resting";
        case State::Captured: return os << "Captured";
    }
    return os << "State(?)";
}

std::ostream& operator<<(std::ostream& os, const Piece& p) {
    return os << "Piece(id=" << p.getId()
              << ", color=" << p.getColor()
              << ", kind=" << p.getKind()
              << ", cell=" << p.getCell()
              << ", state=" << p.getState() << ")";
}

}  // namespace kfc::model
