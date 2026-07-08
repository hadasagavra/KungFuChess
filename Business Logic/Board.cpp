#include "Board.h"

#include <utility>

namespace kfc::logic {

bool operator==(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

Board::Board(std::vector<Row> rows) : rows_(std::move(rows)) {}

int Board::height() const {
    return static_cast<int>(rows_.size());
}

int Board::width() const {
    return rows_.empty() ? 0 : static_cast<int>(rows_.front().size());
}

bool Board::inBounds(Position p) const {
    return p.row >= 0 && p.row < height() && p.col >= 0 && p.col < width();
}

const std::string& Board::at(Position p) const {
    return rows_[p.row][p.col];
}

bool Board::isEmpty(Position p) const {
    return at(p) == kEmptyCellToken;
}

char Board::colorAt(Position p) const {
    return at(p)[0];
}

PieceType Board::pieceTypeAt(Position p) const {
    return *charToPieceType(at(p)[1]);
}

bool Board::isKing(Position p) const {
    return !isEmpty(p) && pieceTypeAt(p) == PieceType::King;
}

bool Board::sameColor(Position a, Position b) const {
    return colorAt(a) == colorAt(b);
}

void Board::movePiece(Position from, Position to) {
    rows_[to.row][to.col] = rows_[from.row][from.col];
    rows_[from.row][from.col] = kEmptyCellToken;
}

void Board::setPieceType(Position p, PieceType type) {
    rows_[p.row][p.col][1] = pieceTypeToChar(type);
}

void Board::print(std::ostream& out) const {
    for (const auto& row : rows_) {
        for (size_t col = 0; col < row.size(); ++col) {
            if (col > 0) out << ' ';
            out << row[col];
        }
        out << '\n';
    }
}

}  // namespace kfc::logic