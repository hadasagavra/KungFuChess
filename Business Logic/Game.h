#pragma once

#include <optional>

#include "Board.h"

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
    bool hasPieceInTransit() const;
    void trySelect(Position p);
    bool tryReselectSameColor(Position p);
    void tryRequestMove(Position p);
    void applyArrivedMove();

    Board board_;
    std::optional<Position> selected_;
    long long clockMs_ = 0;
    std::optional<PendingMove> pending_;
};

}  // namespace kfc::logic