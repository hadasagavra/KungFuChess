#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "PieceTypes.h"

namespace kfc::logic {

using Row = std::vector<std::string>;

struct Position {
    int row;
    int col;
};

bool operator==(Position a, Position b);

class Board {
public:
    explicit Board(std::vector<Row> rows);

    int height() const;
    int width() const;

    bool inBounds(Position p) const;
    bool isEmpty(Position p) const;
    char colorAt(Position p) const;
    PieceType pieceTypeAt(Position p) const;
    bool isKing(Position p) const;
    bool sameColor(Position a, Position b) const;

    void movePiece(Position from, Position to);
    void setPieceType(Position p, PieceType type);
    void print(std::ostream& out) const;

private:
    const std::string& at(Position p) const;

    std::vector<Row> rows_;
};

}  // namespace kfc::logic