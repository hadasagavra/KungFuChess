#pragma once

#include <memory>
#include <vector>

#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/model/include/board_errors.hpp" // הוספת קובץ השגיאות החדש

namespace kfc::model {

// Board is the authoritative, rules-free owner of piece locations. It stores a
// flat grid of shared_ptr<Piece> (shared with the RealTimeArbiter) and keeps
// the grid and each piece's own cell in sync (single source of truth). It has
// ZERO knowledge of chess rules: movePiece assumes validation already occurred.
//
// Throws OutOfBoundsError, CellOccupiedError, or CellEmptyError on contract violations.
class Board {
public:
    Board(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    bool isInsideBounds(Position pos) const;
    bool isOccupied(Position pos) const;

    void addPiece(std::shared_ptr<Piece> piece);
    void removePiece(Position pos);
    std::shared_ptr<Piece> getPieceAt(Position pos) const;
    void movePiece(Position from, Position to);

    // Set the lifecycle state of the piece occupying a cell (used by the arbiter).
    void setPieceState(Position pos, State state);

private:
    int indexOf(Position pos) const;
    void ensureInsideBounds(Position pos) const;

    int width_;
    int height_;
    std::vector<std::shared_ptr<Piece>> grid_;
};

}  // namespace kfc::model