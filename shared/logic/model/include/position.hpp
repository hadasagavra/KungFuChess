#pragma once

#include <iosfwd>

namespace kfc::model {

// A Position is a value object: a single cell coordinate on the board.
// It carries no behavior beyond identity and a readable representation, and it
// deliberately knows nothing about board size, rendering, movement rules, or
// input pixels. Board-bounds checking lives in Board, not here.
struct Position {
    int row;
    int col;
};

bool operator==(const Position& a, const Position& b);
bool operator!=(const Position& a, const Position& b);

// Lexicographic ordering (row, then col) so Position can be used as a key in
// ordered containers such as std::set. Carries no board semantics.
bool operator<(const Position& a, const Position& b);

std::ostream& operator<<(std::ostream& os, const Position& p);

}  // namespace kfc::model
