#include "rules/include/piece_rules.hpp"

#include <cstddef>
#include <set>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::rules {

using model::Board;
using model::Color;
using model::Piece;
using model::Position;

namespace {

// A single-step direction/offset (row delta, col delta).
struct Step {
    int dRow;
    int dCol;
};

constexpr Step kOrthogonal[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr Step kDiagonal[] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr Step kAllDirections[] = {{1, 0},  {-1, 0}, {0, 1},  {0, -1},
                                   {1, 1},  {1, -1}, {-1, 1}, {-1, -1}};
constexpr Step kKnight[] = {{2, 1},  {2, -1},  {-2, 1},  {-2, -1},
                            {1, 2},  {1, -2},  {-1, 2},  {-1, -2}};
constexpr int kDiagonalColumns[] = {-1, 1};

enum class Occupant { Empty, Friendly, Enemy, OffBoard };

// Classify a target cell relative to the moving piece. Bounds are checked first
// so we never trigger Board::getPieceAt's out-of-range throw.
Occupant classify(const Board& board, const Piece& mover, Position pos) {
    if (!board.isInsideBounds(pos)) {
        return Occupant::OffBoard;
    }
    const std::shared_ptr<Piece> other = board.getPieceAt(pos);
    if (!other) {
        return Occupant::Empty;
    }
    return other->getColor() == mover.getColor() ? Occupant::Friendly
                                                 : Occupant::Enemy;
}

Position step(Position from, const Step& dir) {
    return Position{from.row + dir.dRow, from.col + dir.dCol};
}

// Slide outward along each direction until blocked. Empty cells are added and
// traversal continues; an enemy is added but stops traversal (no passing
// through); a friendly or off-board cell stops without being added.
template <std::size_t N>
std::set<Position> slide(const Board& board, const Piece& piece,
                         const Step (&directions)[N]) {
    std::set<Position> destinations;
    const Position from = piece.getCell();
    for (std::size_t i = 0; i < N; ++i) {
        Position current = step(from, directions[i]);
        while (true) {
            const Occupant occupant = classify(board, piece, current);
            if (occupant == Occupant::OffBoard || occupant == Occupant::Friendly) {
                break;
            }
            destinations.insert(current);
            if (occupant == Occupant::Enemy) {
                break;
            }
            current = step(current, directions[i]);
        }
    }
    return destinations;
}

// Single-hop moves (knight, king): a target is a destination if it is empty or
// holds an enemy. Cells between the origin and target are irrelevant, so the
// knight effectively jumps over obstructions.
template <std::size_t N>
std::set<Position> singleSteps(const Board& board, const Piece& piece,
                               const Step (&offsets)[N]) {
    std::set<Position> destinations;
    const Position from = piece.getCell();
    for (std::size_t i = 0; i < N; ++i) {
        const Position target = step(from, offsets[i]);
        const Occupant occupant = classify(board, piece, target);
        if (occupant == Occupant::Empty || occupant == Occupant::Enemy) {
            destinations.insert(target);
        }
    }
    return destinations;
}

// Forward row direction for a pawn: White advances to increasing rows, Black to
// decreasing rows (per board orientation).
int forwardStep(Color color) {
    return color == Color::White ? 1 : -1;
}

}  // namespace

std::set<Position> RookRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    return slide(board, piece, kOrthogonal);
}

std::set<Position> BishopRules::legalDestinations(const Board& board,
                                                  const Piece& piece) const {
    return slide(board, piece, kDiagonal);
}

std::set<Position> QueenRules::legalDestinations(const Board& board,
                                                 const Piece& piece) const {
    return slide(board, piece, kAllDirections);
}

std::set<Position> KnightRules::legalDestinations(const Board& board,
                                                  const Piece& piece) const {
    return singleSteps(board, piece, kKnight);
}

std::set<Position> KingRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    return singleSteps(board, piece, kAllDirections);
}

std::set<Position> PawnRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    std::set<Position> destinations;
    const Position from = piece.getCell();
    const int dir = forwardStep(piece.getColor());

    // Forward one square: allowed only onto an empty cell (never a capture).
    const Position ahead{from.row + dir, from.col};
    if (classify(board, piece, ahead) == Occupant::Empty) {
        destinations.insert(ahead);
    }

    // Diagonal captures: allowed only onto an enemy-occupied cell.
    for (const int dCol : kDiagonalColumns) {
        const Position diagonal{from.row + dir, from.col + dCol};
        if (classify(board, piece, diagonal) == Occupant::Enemy) {
            destinations.insert(diagonal);
        }
    }
    return destinations;
}

}  // namespace kfc::rules
