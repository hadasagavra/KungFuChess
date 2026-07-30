#include "client/app/include/game_app.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

#include "client/view/include/scene_translator.hpp"
#include "shared/frame_step.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::app {
namespace {

// Largest real-time step fed to the engine in one frame (see clampedStepMs).
constexpr int maxStepMs = 100;

// Default seat labels shown when the game has not named the players (local play,
// or before a networked roster arrives).
const char* const whitePlayerName = "White";
const char* const blackPlayerName = "Black";

// A player's name if the game knows it, otherwise the given default label.
std::string nameOr(const std::string& name, const char* fallback) {
    return name.empty() ? fallback : name;
}

}  // namespace

GameApp::GameApp(net::ClientGame& game, int boardWidth, int boardHeight,
                 const std::string& assetsRoot, int cellPx)
    : game_(game),
      config_(view::defaultRenderConfig(assetsRoot, cellPx)),
      // One layout answer, shared: the renderer draws the board at this origin
      // and the mapper reads clicks against it, so the two cannot disagree.
      layout_(view::computeLayout(config_, boardWidth, boardHeight)),
      mapper_(boardWidth, boardHeight, cellPx, layout_.boardOrigin.x,
              layout_.boardOrigin.y),
      controller_(game_, mapper_),
      lobbyController_(game_),
      view_(config_) {}

void GameApp::subscribeRecords() {
    // The moves log and the score are Business Logic that listens: the game
    // publishes what happened and never learns who is recording it. This is the
    // whole cost of a feature of this kind, identical whether the event was born
    // in a local engine or arrived over the wire.
    game_.events().subscribe<model::MoveEvent>(
        [this](const model::MoveEvent& event) { moveLog_.record(event); });
    game_.events().subscribe<model::CapturedPiece>(
        [this](const model::CapturedPiece& captured) {
            scoreBoard_.record(captured);
        });
}

int GameApp::run() {
    subscribeRecords();
    view_.open();

    auto last = std::chrono::steady_clock::now();
    while (view_.isOpen()) {
        const auto now = std::chrono::steady_clock::now();
        const int deltaMs = clampedStepMs(last, now, maxStepMs);
        game_.advance(deltaMs);
        last = now;

        // A refused login (wrong password) ends the session: report it and stop,
        // rather than sit in a window that will never be seated.
        if (const std::optional<std::string> error = game_.authError()) {
            std::cout << "LOGIN FAILED: " << *error << "\n";
            return 1;
        }

        // The Home screen is shown until the server puts this client in a game.
        // Local play is always "in game", so it never lands here.
        if (!game_.isInGame()) {
            drawLobby();
        } else {
            drawGame(deltaMs);
        }
    }
    return 0;
}

void GameApp::drawLobby() {
    view_.renderLobby(lobbyController_.view());
    for (const int key : view_.takeKeys()) lobbyController_.handleKey(key);
    for (const view::MouseAction& action : view_.takeMouseActions()) {
        if (action.type == view::MouseAction::Type::Click) {
            lobbyController_.handleClick(action.position);
        }
    }
}

void GameApp::drawGame(int deltaMs) {
    // Highlight the selected piece's legal destinations, if any. The Controller
    // owns the selection; the game answers where that piece may move.
    std::set<model::Position> highlights;
    if (const std::optional<model::Position>& selected = controller_.selection()) {
        highlights = game_.legalDestinationsFor(*selected);
    }

    const engine::GameSnapshot state = game_.getSnapshot();
    // Names and ratings come from the game when it knows them (a networked
    // roster); otherwise the default labels stand and no rating is shown.
    const view::SceneInput scene{state,
                                 moveLog_,
                                 scoreBoard_,
                                 highlights,
                                 nameOr(game_.whiteName(), whitePlayerName),
                                 nameOr(game_.blackName(), blackPlayerName),
                                 game_.whiteRating(),
                                 game_.blackRating()};
    view::GameSnapshot drawable = view::buildSnapshot(scene, config_, layout_);
    // The room id rides at the top of the board, with a disconnected opponent's
    // forfeit countdown over it.
    if (!game_.roomId().empty()) drawable.roomBanner = "Room: " + game_.roomId();
    drawable.opponentCountdown = game_.opponentCountdown();
    view_.render(drawable, deltaMs);

    dispatchBoardInput();
}

void GameApp::dispatchBoardInput() {
    // A double-click requests a jump; a single click drives the ordinary
    // select/move flow. A double-click also delivers the opening click that began
    // it, so a click immediately followed by a double-click on the same spot is
    // dropped -- acting on it too would leave a stray selection on the piece that
    // just jumped.
    const std::vector<view::MouseAction> actions = view_.takeMouseActions();
    for (std::size_t i = 0; i < actions.size(); ++i) {
        const view::MouseAction& action = actions[i];
        if (action.type == view::MouseAction::Type::DoubleClick) {
            controller_.handleJump(action.position.x, action.position.y);
            continue;
        }
        const bool opensDoubleClick =
            i + 1 < actions.size() &&
            actions[i + 1].type == view::MouseAction::Type::DoubleClick &&
            actions[i + 1].position == action.position;
        if (!opensDoubleClick) {
            controller_.handleClick(action.position.x, action.position.y);
        }
    }
}

}  // namespace kfc::app
