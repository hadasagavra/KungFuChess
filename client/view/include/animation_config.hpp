#pragma once

#include <functional>

#include "shared/logic/model/include/piece.hpp"

namespace kfc::view {

// The 1-based number of the first sprite file on disk (sprites/1.png). Sprite
// frames are numbered from here so the Animator and the Renderer agree on the
// file-naming scheme in exactly one place.
const int firstSpriteFrame = 1;

// Pure display metadata for one piece-state's animation, read from that state's
// config.json plus the number of sprite files on disk. It carries only what the
// view needs to play frames -- frames_per_sec and is_loop from the config's
// "graphics" section. The config's "physics" (speed, next_state) is a Business
// Logic concern and is deliberately absent here, so the view never encodes game
// rules.
struct AnimationConfig {
    int framesPerSec;
    bool isLoop;
    int frameCount;
};

// How the Animator looks up the config for a given piece-state. Injected as a
// callable so the Animator stays pure and unit-testable: production binds it to
// the disk-backed AnimationConfigStore; tests bind a stub with no file access.
using AnimationConfigProvider =
    std::function<AnimationConfig(model::Kind, model::Color, model::State)>;

}  // namespace kfc::view
