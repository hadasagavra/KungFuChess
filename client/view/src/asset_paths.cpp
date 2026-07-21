#include "client/view/include/asset_paths.hpp"

#include <string>

namespace kfc::view {
namespace {

// The two halves of a piece folder code, e.g. Pawn + White -> 'P' + 'W' = "PW".
char pieceLetter(model::Kind kind) {
    switch (kind) {
        case model::Kind::King:   return 'K';
        case model::Kind::Queen:  return 'Q';
        case model::Kind::Rook:   return 'R';
        case model::Kind::Bishop: return 'B';
        case model::Kind::Knight: return 'N';
        case model::Kind::Pawn:   return 'P';
    }
    return '?';
}

char colorLetter(model::Color color) {
    switch (color) {
        case model::Color::White: return 'W';
        case model::Color::Black: return 'B';
    }
    return '?';
}

}  // namespace

std::string pieceCode(model::Kind kind, model::Color color) {
    return std::string{pieceLetter(kind), colorLetter(color)};
}

std::string stateFolder(model::State state) {
    switch (state) {
        case model::State::Idle:     return "idle";
        case model::State::Moving:   return "move";
        case model::State::Airborne: return "jump";
        case model::State::Resting:  return "short_rest";  // provisional; see note
        case model::State::Captured: return "idle";        // not drawn in practice
    }
    return "idle";
}

std::string stateDir(const std::string& assetsRoot, model::Kind kind,
                     model::Color color, model::State state) {
    return assetsRoot + "/pieces2/" + pieceCode(kind, color) + "/states/" +
           stateFolder(state);
}

std::string configPath(const std::string& assetsRoot, model::Kind kind,
                       model::Color color, model::State state) {
    return stateDir(assetsRoot, kind, color, state) + "/config.json";
}

std::string spriteFramePath(const std::string& assetsRoot, model::Kind kind,
                            model::Color color, model::State state, int frame) {
    return stateDir(assetsRoot, kind, color, state) + "/sprites/" +
           std::to_string(frame) + ".png";
}

}  // namespace kfc::view
