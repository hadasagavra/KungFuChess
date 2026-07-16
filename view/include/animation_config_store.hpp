#pragma once

#include <string>
#include <unordered_map>

#include "model/include/piece.hpp"
#include "view/include/animation_config.hpp"

namespace kfc::view {

// Loads and caches the per-state AnimationConfig from disk: it reads each state's
// config.json (the graphics section) and counts the sprite files. This is the
// impure, asset-loading side of the animation seam -- it is the only place that
// touches the filesystem, so the Animator that consumes it stays pure. Each
// (piece, state) config is read once and memoized, so playback never hits the
// disk per frame.
class AnimationConfigStore {
public:
    explicit AnimationConfigStore(std::string assetsRoot);

    // The config for a piece-state, loading and caching it on first request.
    AnimationConfig configFor(model::Kind kind, model::Color color,
                              model::State state);

private:
    AnimationConfig load(model::Kind kind, model::Color color,
                         model::State state) const;

    std::string assetsRoot_;
    std::unordered_map<std::string, AnimationConfig> cache_;
};

}  // namespace kfc::view
