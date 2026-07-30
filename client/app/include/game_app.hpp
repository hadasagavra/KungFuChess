#pragma once

#include <string>

#include "client/app/include/lobby_controller.hpp"
#include "client/input/include/board_mapper.hpp"
#include "client/input/include/controller.hpp"
#include "client/net/include/client_game.hpp"
#include "client/view/include/image_view.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"
#include "shared/logic/game_record/include/move_log.hpp"
#include "shared/logic/game_record/include/score_board.hpp"

namespace kfc::app {

// The graphical client application: a persistent window that renders the game's
// live state every frame and turns mouse and keyboard input into actions. It
// drives a ClientGame and neither knows nor cares whether the authority is a
// same-process loopback or a socket. It shows the Home screen (through a
// LobbyController) until the server seats this client, then draws the board.
//
// This is the client's orchestration layer -- the counterpart to the server's
// RoomManager loop -- lifted out of the composition root so main is a thin shim.
class GameApp {
public:
    GameApp(net::ClientGame& game, int boardWidth, int boardHeight,
            const std::string& assetsRoot, int cellPx);

    // Run the frame loop until the window closes (or a login is refused). Returns
    // a process exit code.
    int run();

private:
    // Subscribe the moves log and score to the game's event stream.
    void subscribeRecords();
    // One Home-screen frame: draw it, then feed clicks/keys to the controller.
    void drawLobby();
    // One in-game frame: draw the board with its overlays, then dispatch clicks.
    void drawGame(int deltaMs);
    void dispatchBoardInput();

    net::ClientGame& game_;
    game_record::MoveLog moveLog_;
    game_record::ScoreBoard scoreBoard_;
    view::RenderConfig config_;
    view::FrameLayout layout_;
    input::BoardMapper mapper_;
    input::Controller controller_;
    LobbyController lobbyController_;
    view::ImageView view_;
};

}  // namespace kfc::app
