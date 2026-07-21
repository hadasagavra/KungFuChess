#include "third_party/doctest/doctest.h"

#include <vector>

#include "engine/include/game_observer.hpp"
#include "engine/include/game_subject.hpp"
#include "model/include/game_event.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::engine::GameObserver;
using kfc::engine::GameSubject;
using kfc::model::CapturedPiece;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::model::Position;

namespace {

// Records what it was told, so a test can check what actually reached it.
class RecordingObserver : public GameObserver {
public:
    void onMove(const MoveEvent& event) override { moves.push_back(event); }
    void onCapture(const CapturedPiece& captured) override {
        captures.push_back(captured);
    }

    std::vector<MoveEvent> moves;
    std::vector<CapturedPiece> captures;
};

// Overrides neither callback: the defaults must absorb both events silently.
class IndifferentObserver : public GameObserver {};

MoveEvent someMove() {
    return MoveEvent{500,           Color::White,   Kind::Pawn,
                     Position{6, 0}, Position{5, 0}, false, false};
}

}  // namespace

TEST_CASE("publishing with no observers is harmless") {
    const GameSubject subject;

    subject.publishMove(someMove());
    subject.publishCapture(CapturedPiece{Kind::Pawn, Color::Black});
}

TEST_CASE("a registered observer receives published events") {
    GameSubject subject;
    RecordingObserver observer;
    subject.addObserver(observer);

    subject.publishMove(someMove());
    subject.publishCapture(CapturedPiece{Kind::Rook, Color::Black});

    REQUIRE(observer.moves.size() == 1);
    CHECK(observer.moves[0].timeMs == 500);
    REQUIRE(observer.captures.size() == 1);
    CHECK(observer.captures[0].kind == Kind::Rook);
}

TEST_CASE("every observer receives every event") {
    GameSubject subject;
    RecordingObserver first;
    RecordingObserver second;
    subject.addObserver(first);
    subject.addObserver(second);

    subject.publishMove(someMove());

    CHECK(first.moves.size() == 1);
    CHECK(second.moves.size() == 1);
}

TEST_CASE("an observer may ignore the events it does not care about") {
    GameSubject subject;
    IndifferentObserver indifferent;
    RecordingObserver recording;
    subject.addObserver(indifferent);
    subject.addObserver(recording);

    subject.publishMove(someMove());
    subject.publishCapture(CapturedPiece{Kind::Pawn, Color::White});

    // The indifferent observer swallowed both without disturbing delivery to
    // the one that does care.
    CHECK(recording.moves.size() == 1);
    CHECK(recording.captures.size() == 1);
}
