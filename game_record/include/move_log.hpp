#pragma once

#include <vector>

#include "engine/include/game_observer.hpp"
#include "model/include/game_event.hpp"
#include "model/include/piece.hpp"

namespace kfc::game_record {

// The game's move history, kept per player and in the order the moves were
// ordered. It observes the engine rather than being driven by it, so recording
// history costs the engine nothing but one publish call.
//
// It stores the MoveEvent itself: an entry IS the event that was published, so
// there is no second near-identical record type to keep in step with it.
// Notation and clock formatting are not done here -- a log entry stays domain
// data, and the display boundary renders it.
class MoveLog : public engine::GameObserver {
public:
    void onMove(const model::MoveEvent& event) override;

    // The moves that player ordered, oldest first.
    const std::vector<model::MoveEvent>& entriesFor(model::Color player) const;

private:
    std::vector<model::MoveEvent>& bucketFor(model::Color player);

    std::vector<model::MoveEvent> white_;
    std::vector<model::MoveEvent> black_;
};

}  // namespace kfc::game_record
