#pragma once

#include <optional>
#include <vector>

#include "Board.hpp"

namespace kfc::logic {

struct PendingMove {
    Position from;
    Position to;
    long long arrivalMs;
};

class Game {
public:
    explicit Game(Board board);

    void handleClickCell(Position p);
    void advanceClock(long long ms);
    const Board& board() const;

private:
    bool isPieceInTransitAt(Position p) const;
    void tryRequestMove(Position to);
    void applyArrivedMoves();
    bool canLand(Position from, Position to) const;
    void promoteIfNeeded(Position p);

    Board board_;
    std::optional<Position> selected_;
    long long clockMs_ = 0;
    std::vector<PendingMove> activeMoves_;
    bool gameOver_ = false;
};

}  // namespace kfc::logic