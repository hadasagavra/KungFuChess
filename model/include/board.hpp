#pragma once

#include <memory>
#include <vector>

#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::model {

// Board is the authoritative, rules-free owner of piece locations. It stores a
// flat grid of shared_ptr<Piece> (shared with the RealTimeArbiter) and keeps
// the grid and each piece's own cell in sync (single source of truth). It has
// ZERO knowledge of chess rules: movePiece assumes validation already occurred.
//
// width = number of columns, height = number of rows; a Position{row, col}
// maps to row * width + col.
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

private:
    int indexOf(Position pos) const;
    void ensureInsideBounds(Position pos) const;

    int width_;
    int height_;
    std::vector<std::shared_ptr<Piece>> grid_;
};

}  // namespace kfc::model
