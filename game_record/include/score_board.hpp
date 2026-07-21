#pragma once

#include "model/include/game_event.hpp"
#include "model/include/piece.hpp"

namespace kfc::game_record {

// Each player's score: the summed cost of the pieces they have captured. This
// class is the single implementation of that rule -- nothing else anywhere adds
// up piece costs, least of all the display, which only reads the totals.
//
// It knows nothing about how a capture reaches it. Whoever holds one subscribes
// it to the bus; it would work just as well driven by a replay or a server.
class ScoreBoard {
public:
    void record(const model::CapturedPiece& captured);

    int scoreFor(model::Color player) const;

private:
    int& totalFor(model::Color player);

    int white_ = 0;
    int black_ = 0;
};

}  // namespace kfc::game_record
