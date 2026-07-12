#include "model/include/position.hpp"

#include <ostream>

namespace kfc::model {

bool operator==(const Position& a, const Position& b) {
    return a.row == b.row && a.col == b.col;
}

bool operator!=(const Position& a, const Position& b) {
    return !(a == b);
}

bool operator<(const Position& a, const Position& b) {
    return a.row != b.row ? a.row < b.row : a.col < b.col;
}

std::ostream& operator<<(std::ostream& os, const Position& p) {
    return os << "Position(row=" << p.row << ", col=" << p.col << ")";
}

}  // namespace kfc::model
