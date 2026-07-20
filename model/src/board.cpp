#include "model/include/board.hpp"

#include <stdexcept>

namespace kfc::model {

Board::Board(int width, int height) : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0) {
        throw std::invalid_argument("Board: dimensions must be positive");
    }
    grid_.resize(static_cast<std::size_t>(width_) * height_);
}

int Board::indexOf(Position pos) const {
    return pos.row * width_ + pos.col;
}

void Board::ensureInsideBounds(Position pos) const {
    if (!isInsideBounds(pos)) {
        throw OutOfBoundsError("Board: position out of bounds");
    }
}

bool Board::isInsideBounds(Position pos) const {
    return pos.row >= 0 && pos.row < height_ && pos.col >= 0 && pos.col < width_;
}

bool Board::isOccupied(Position pos) const {
    ensureInsideBounds(pos);
    return grid_[indexOf(pos)] != nullptr;
}

std::shared_ptr<Piece> Board::getPieceAt(Position pos) const {
    ensureInsideBounds(pos);
    return grid_[indexOf(pos)];
}

void Board::addPiece(std::shared_ptr<Piece> piece) {
    if (!piece) {
        throw std::invalid_argument("Board::addPiece: piece is null");
    }
    const Position pos = piece->getCell();
    if (!isInsideBounds(pos)) {
        throw OutOfBoundsError("Board::addPiece: cell out of bounds");
    }
    if (grid_[indexOf(pos)] != nullptr) {
        throw CellOccupiedError("Board::addPiece: cell occupied");
    }
    grid_[indexOf(pos)] = std::move(piece);
}

void Board::removePiece(Position pos) {
    ensureInsideBounds(pos);
    grid_[indexOf(pos)] = nullptr;
}

void Board::movePiece(Position from, Position to) {
    ensureInsideBounds(from);
    ensureInsideBounds(to);

    std::shared_ptr<Piece> piece = grid_[indexOf(from)];
    if (!piece) {
        throw CellEmptyError("Board::movePiece: origin cell is empty");
    }
    if (grid_[indexOf(to)] != nullptr) {
        throw CellOccupiedError("Board::movePiece: destination cell occupied");
    }

    grid_[indexOf(to)] = piece;
    grid_[indexOf(from)] = nullptr;
    piece->setCell(to);
}

void Board::setPieceState(Position pos, State state) {
    ensureInsideBounds(pos);
    const std::shared_ptr<Piece> piece = grid_[indexOf(pos)];
    if (!piece) {
        throw CellEmptyError("Board::setPieceState: cell is empty");
    }
    piece->setState(state);
}

}  // namespace kfc::model