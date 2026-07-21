#include "third_party/doctest/doctest.h"

#include <cstdint>

#include "shared/logic/model/include/piece.hpp"
#include "client/view/include/animation_config.hpp"
#include "client/view/include/animator.hpp"
#include "client/view/include/game_snapshot.hpp"

using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::State;
using kfc::view::AnimationConfig;
using kfc::view::Animator;
using kfc::view::firstSpriteFrame;
using kfc::view::frameForElapsed;
using kfc::view::GameSnapshot;
using kfc::view::PieceView;

namespace {

// A snapshot holding a single piece with the given id/state, so tests can drive
// the Animator one piece at a time.
GameSnapshot onePiece(std::uint32_t id, State state) {
    GameSnapshot snapshot;
    snapshot.boardWidth = 8;
    snapshot.boardHeight = 8;
    snapshot.pieces.push_back(
        PieceView{Kind::Pawn, Color::White, state, {0, 0}, id});
    return snapshot;
}

// A provider that returns the same config for every piece-state, so a test
// controls fps/loop/frameCount without touching the disk.
kfc::view::AnimationConfigProvider fixedConfig(AnimationConfig config) {
    return [config](Kind, Color, State) { return config; };
}

int frameOf(const GameSnapshot& snapshot) {
    return snapshot.pieces.front().frame;
}

}  // namespace

TEST_CASE("frameForElapsed cycles a looping animation") {
    // 10 fps => 100 ms per frame, 3 frames, looping.
    const AnimationConfig config{10, true, 3};
    CHECK(frameForElapsed(config, 0) == firstSpriteFrame);
    CHECK(frameForElapsed(config, 100) == firstSpriteFrame + 1);
    CHECK(frameForElapsed(config, 200) == firstSpriteFrame + 2);
    CHECK(frameForElapsed(config, 300) == firstSpriteFrame);  // wrapped
}

TEST_CASE("frameForElapsed holds the last frame of a non-looping animation") {
    const AnimationConfig config{10, false, 3};
    CHECK(frameForElapsed(config, 200) == firstSpriteFrame + 2);
    CHECK(frameForElapsed(config, 999) == firstSpriteFrame + 2);  // held
}

TEST_CASE("frameForElapsed guards degenerate configs") {
    CHECK(frameForElapsed(AnimationConfig{0, true, 3}, 500) == firstSpriteFrame);
    CHECK(frameForElapsed(AnimationConfig{10, true, 0}, 500) == firstSpriteFrame);
}

TEST_CASE("Animator advances a piece's frame as time passes") {
    Animator animator{fixedConfig(AnimationConfig{10, true, 3})};

    CHECK(frameOf(animator.animate(onePiece(1, State::Idle), 0)) ==
          firstSpriteFrame);
    CHECK(frameOf(animator.animate(onePiece(1, State::Idle), 100)) ==
          firstSpriteFrame + 1);
    CHECK(frameOf(animator.animate(onePiece(1, State::Idle), 100)) ==
          firstSpriteFrame + 2);
}

TEST_CASE("Animator restarts the animation when a piece changes state") {
    Animator animator{fixedConfig(AnimationConfig{10, true, 3})};

    animator.animate(onePiece(1, State::Idle), 100);
    animator.animate(onePiece(1, State::Idle), 100);  // now on a later frame

    // Switching state resets elapsed time back to the first frame.
    CHECK(frameOf(animator.animate(onePiece(1, State::Moving), 0)) ==
          firstSpriteFrame);
}

TEST_CASE("Animator tracks pieces independently by id") {
    Animator animator{fixedConfig(AnimationConfig{10, true, 3})};

    GameSnapshot both;
    both.boardWidth = 8;
    both.boardHeight = 8;
    both.pieces.push_back(PieceView{Kind::Pawn, Color::White, State::Idle,
                                    {0, 0}, /*id=*/1});
    both.pieces.push_back(PieceView{Kind::Pawn, Color::Black, State::Idle,
                                    {100, 0}, /*id=*/2});

    animator.animate(both, 0);  // both pieces start at frame one
    GameSnapshot advanced = animator.animate(both, 100);
    CHECK(advanced.pieces[0].frame == firstSpriteFrame + 1);
    CHECK(advanced.pieces[1].frame == firstSpriteFrame + 1);
}

TEST_CASE("Animator forgets a captured piece so a reused id starts fresh") {
    Animator animator{fixedConfig(AnimationConfig{10, true, 3})};

    animator.animate(onePiece(1, State::Idle), 100);
    animator.animate(onePiece(1, State::Idle), 100);  // piece 1 advanced

    // Piece 1 leaves the board for a tick (captured): an empty snapshot.
    GameSnapshot empty;
    empty.boardWidth = 8;
    empty.boardHeight = 8;
    animator.animate(empty, 100);

    // A new piece reusing id 1 starts its animation from the first frame.
    CHECK(frameOf(animator.animate(onePiece(1, State::Idle), 0)) ==
          firstSpriteFrame);
}
