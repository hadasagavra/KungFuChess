#include "engine/include/game_subject.hpp"

namespace kfc::engine {

void GameSubject::addObserver(GameObserver& observer) {
    observers_.push_back(&observer);
}

void GameSubject::publishMove(const model::MoveEvent& event) const {
    for (GameObserver* observer : observers_) {
        observer->onMove(event);
    }
}

void GameSubject::publishCapture(const model::CapturedPiece& captured) const {
    for (GameObserver* observer : observers_) {
        observer->onCapture(captured);
    }
}

}  // namespace kfc::engine
