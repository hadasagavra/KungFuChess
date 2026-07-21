#pragma once

#include <string>

#include "shared/logic/model/include/piece.hpp"

namespace kfc::view {

// Single source of truth for the sprite asset layout. These map the domain
// enums onto the on-disk folder scheme (e.g. Pawn+White -> "PW", Idle ->
// "idle") so no asset-path literals are duplicated across the view.
std::string pieceCode(model::Kind kind, model::Color color);
std::string stateFolder(model::State state);

// Directory holding one piece-state's assets: assetsRoot/pieces2/<code>/states/
// <folder>. Both the sprite path and the config path derive from it, so the
// on-disk layout is spelled out in exactly one place.
std::string stateDir(const std::string& assetsRoot, model::Kind kind,
                     model::Color color, model::State state);
std::string configPath(const std::string& assetsRoot, model::Kind kind,
                       model::Color color, model::State state);
std::string spriteFramePath(const std::string& assetsRoot, model::Kind kind,
                            model::Color color, model::State state, int frame);

}  // namespace kfc::view
