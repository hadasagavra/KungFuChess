#include "game_record/include/move_log.hpp"

namespace kfc::game_record {

using model::Color;
using model::MoveEvent;

void MoveLog::onMove(const MoveEvent& event) {
    bucketFor(event.player).push_back(event);
}

const std::vector<MoveEvent>& MoveLog::entriesFor(Color player) const {
    return player == Color::White ? white_ : black_;
}

std::vector<MoveEvent>& MoveLog::bucketFor(Color player) {
    return player == Color::White ? white_ : black_;
}

}  // namespace kfc::game_record
