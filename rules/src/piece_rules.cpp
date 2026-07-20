#include "rules/include/piece_rules.hpp"

#include <cstddef>
#include <set>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::rules {

using model::Board;
using model::Color;
using model::Kind;
using model::Piece;
using model::Position;

namespace {

// A single-step direction/offset (row delta, col delta).
struct Step {
    int dRow;
    int dCol;
};

constexpr Step orthogonal[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr Step diagonal[] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr Step allDirections[] = {{1, 0},  {-1, 0}, {0, 1},  {0, -1},
                                   {1, 1},  {1, -1}, {-1, 1}, {-1, -1}};
constexpr Step knight[] = {{2, 1},  {2, -1},  {-2, 1},  {-2, -1},
                            {1, 2},  {1, -2},  {-1, 2},  {-1, -2}};
constexpr int diagonalColumns[] = {-1, 1};

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

// Forward row direction for a pawn: White advances up the board (toward row 0),
// Black advances down the board (toward higher row indices).
constexpr int whiteForward = -1;
constexpr int blackForward = 1;
constexpr int blackStartRow = 1;
constexpr int whiteStartOffset = 2;  // white pawns start on row height - 2
constexpr int pawnDoubleStep = 2;

int forwardStep(Color color) {
    return color == Color::White ? whiteForward : blackForward;
}

// The row a pawn of this color starts on (where a double step is allowed).
int startRow(Color color, int height) {
    return color == Color::White ? height - whiteStartOffset : blackStartRow;
}

}  // namespace

std::set<Position> RookRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    return slide(board, piece, orthogonal);
}

std::set<Position> BishopRules::legalDestinations(const Board& board,
                                                  const Piece& piece) const {
    return slide(board, piece, diagonal);
}

std::set<Position> QueenRules::legalDestinations(const Board& board,
                                                 const Piece& piece) const {
    return slide(board, piece, allDirections);
}

std::set<Position> KnightRules::legalDestinations(const Board& board,
                                                  const Piece& piece) const {
    return singleSteps(board, piece, knight);
}

std::set<Position> KingRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    return singleSteps(board, piece, allDirections);
}

std::set<Position> PawnRules::legalDestinations(const Board& board,
                                                const Piece& piece) const {
    std::set<Position> destinations;
    const Position from = piece.getCell();
    const int dir = forwardStep(piece.getColor());

    // Forward one square: allowed only onto an empty cell (never a capture). From
    // the start row, a double step is allowed when both squares ahead are empty.
    const Position ahead{from.row + dir, from.col};
    if (classify(board, piece, ahead) == Occupant::Empty) {
        destinations.insert(ahead);
        if (from.row == startRow(piece.getColor(), board.height())) {
            const Position ahead2{from.row + pawnDoubleStep * dir, from.col};
            if (classify(board, piece, ahead2) == Occupant::Empty) {
                destinations.insert(ahead2);
            }
        }
    }

    // Diagonal captures: allowed only onto an enemy-occupied cell.
    for (const int dCol : diagonalColumns) {
        const Position diagonal{from.row + dir, from.col + dCol};
        if (classify(board, piece, diagonal) == Occupant::Enemy) {
            destinations.insert(diagonal);
        }
    }
    return destinations;
}

const PieceRules& ruleFor(Kind kind) {
    static const RookRules rook;
    static const BishopRules bishop;
    static const QueenRules queen;
    static const KnightRules knight;
    static const KingRules king;
    static const PawnRules pawn;

    switch (kind) {
        case Kind::Rook:   return rook;
        case Kind::Bishop: return bishop;
        case Kind::Queen:  return queen;
        case Kind::Knight: return knight;
        case Kind::King:   return king;
        case Kind::Pawn:   return pawn;
    }
    throw std::invalid_argument("ruleFor: unknown piece kind");
}

std::optional<Kind> promotedKind(const Board& board, const Piece& piece) {
    if (piece.getKind() != Kind::Pawn) {
        return std::nullopt;
    }
    const int promoteRow = forwardStep(piece.getColor()) < 0 ? 0 : board.height() - 1;
    if (piece.getCell().row == promoteRow) {
        return Kind::Queen;
    }
    return std::nullopt;
}

}  // namespace kfc::rules
