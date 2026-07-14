#include "model/include/piece.hpp"

#include <ostream>

namespace kfc::model {

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
