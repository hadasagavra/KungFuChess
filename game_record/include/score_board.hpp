#pragma once

#include "engine/include/game_observer.hpp"
#include "model/include/game_event.hpp"
#include "model/include/piece.hpp"

namespace kfc::game_record {

// Each player's score: the summed cost of the pieces they have captured. This
// class is the single implementation of that rule -- nothing else anywhere adds
// up piece costs, least of all the display, which only reads the totals.
class ScoreBoard : public engine::GameObserver {
public:
    void onCapture(const model::CapturedPiece& captured) override;

    int scoreFor(model::Color player) const;

private:
    int& totalFor(model::Color player);

    int white_ = 0;
    int black_ = 0;
};

}  // namespace kfc::game_record
