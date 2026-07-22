#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "shared/logic/engine/include/game_engine.hpp"
#include "client/input/include/board_mapper.hpp"
#include "client/input/include/controller.hpp"
#include "client/input/include/local_game_access.hpp"
#include "client/net/include/client_game.hpp"
#include "client/net/include/endpoint.hpp"
#include "client/net/include/loopback_game.hpp"
#include "client/net/include/networked_game.hpp"
#include "shared/logic/game_record/include/move_log.hpp"
#include "shared/logic/game_record/include/score_board.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "texttests/include/script_parser.hpp"
#include "texttests/include/script_runner.hpp"
#include "client/view/include/image_view.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"
#include "client/view/include/scene_translator.hpp"

//https://github.com/hadasagavra/KungFuChess

namespace {

// Where the opening position is read from -- the same file the server loads, so
// one description of the board serves both composition roots. Resolved from the
// working directory, like the assets.
const char* const startPositionPath = "config/start_position.txt";

// The default server address for --connect when it is given no address, or only
// a host or only a port.
const char* const defaultHost = "127.0.0.1";
constexpr std::uint16_t defaultPort = 9000;

// Largest real-time step fed to the engine in one frame. Clamping keeps a
// paused/janky frame (or the first frame) from jumping the clock forward.
constexpr int maxStepMs = 100;

// Who is playing. Names are shown beside each player's moves table; no game rule
// depends on them, so they stay here in the composition root rather than in the
// Business Logic.
const char* const whitePlayerName = "White";
const char* const blackPlayerName = "Black";

int elapsedMsSince(std::chrono::steady_clock::time_point last,
                   std::chrono::steady_clock::time_point now) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    return std::clamp(static_cast<int>(ms), 0, maxStepMs);
}

// Text harness: read a board + a script from `in`, replay the commands through
// the public command path, and print board dumps to `out`. It drives a local
// engine directly through a LocalGameAccess -- it is a test tool, not networked
// play, so it does not go over the server protocol.
int runScript(std::istream& in, std::ostream& out) {
    kfc::io::ParsedInput parsed = kfc::io::parseInput(in);

    kfc::engine::GameEngine engine{parsed.board};
    kfc::input::LocalGameAccess access{engine};
    // The text harness draws nothing, so the board sits at the frame origin with
    // no panels beside it.
    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height(),
                                   kfc::view::defaultCellPx, 0, 0};
    kfc::input::Controller controller{access, mapper};
    kfc::texttests::ScriptRunner runner{controller, engine, out};

    for (const std::string& line : parsed.commands) {
        if (std::optional<kfc::texttests::Command> command =
                kfc::texttests::parseCommand(line)) {
            runner.run(*command);
        }
    }
    return 0;
}

// Graphical play: a persistent window renders the game's live state every frame,
// real time advances by wall-clock, and each click is forwarded to the
// Controller (source -> destination moves). The loop speaks only to a ClientGame
// -- it neither knows nor cares whether the authority sits in this process
// (loopback) or across a socket. The window speaks pixels; the Controller owns
// the pixel->cell mapping, so no board logic lives here.
int runGraphical(kfc::net::ClientGame& game, int boardWidth, int boardHeight,
                 const std::string& assetsRoot, int cellPx) {
    // The moves log and the score are Business Logic that listens: the game
    // publishes what happened and never learns who is recording it. Subscribing
    // here is the whole cost of a feature of this kind -- and it is identical
    // whether the event was born in a local engine or arrived over the wire.
    kfc::game_record::MoveLog moveLog;
    kfc::game_record::ScoreBoard scoreBoard;
    game.events().subscribe<kfc::model::MoveEvent>(
        [&moveLog](const kfc::model::MoveEvent& event) { moveLog.record(event); });
    game.events().subscribe<kfc::model::CapturedPiece>(
        [&scoreBoard](const kfc::model::CapturedPiece& captured) {
            scoreBoard.record(captured);
        });

    const kfc::view::RenderConfig config =
        kfc::view::defaultRenderConfig(assetsRoot, cellPx);
    // One layout answer, shared: the renderer draws the board at this origin and
    // the mapper reads clicks against it, so the two cannot disagree.
    const kfc::view::FrameLayout layout =
        kfc::view::computeLayout(config, boardWidth, boardHeight);

    kfc::input::BoardMapper mapper{boardWidth, boardHeight, cellPx,
                                   layout.boardOrigin.x, layout.boardOrigin.y};
    kfc::input::Controller controller{game, mapper};

    kfc::view::ImageView view{config};
    view.open();

    auto last = std::chrono::steady_clock::now();
    while (view.isOpen()) {
        const auto now = std::chrono::steady_clock::now();
        const int deltaMs = elapsedMsSince(last, now);
        game.advance(deltaMs);
        last = now;

        // Highlight the selected piece's legal destinations, if any. The
        // Controller owns the selection; the game answers where that piece may
        // move. The composition root only plumbs the two together.
        std::set<kfc::model::Position> highlights;
        if (const std::optional<kfc::model::Position>& selected =
                controller.selection()) {
            highlights = game.legalDestinationsFor(*selected);
        }
        const kfc::engine::GameSnapshot state = game.getSnapshot();
        const kfc::view::SceneInput scene{
            state,      moveLog,          scoreBoard,
            highlights, whitePlayerName,  blackPlayerName};
        view.render(kfc::view::buildSnapshot(scene, config, layout), deltaMs);

        // A double-click requests a jump; a single click drives the ordinary
        // select/move flow. A double-click also delivers the opening click that
        // began it, so a click immediately followed by a double-click on the same
        // spot is dropped -- acting on it too would leave a stray selection on
        // the piece that just jumped.
        const std::vector<kfc::view::MouseAction> actions = view.takeMouseActions();
        for (std::size_t i = 0; i < actions.size(); ++i) {
            const kfc::view::MouseAction& action = actions[i];
            if (action.type == kfc::view::MouseAction::Type::DoubleClick) {
                controller.handleJump(action.position.x, action.position.y);
                continue;
            }
            const bool opensDoubleClick =
                i + 1 < actions.size() &&
                actions[i + 1].type == kfc::view::MouseAction::Type::DoubleClick &&
                actions[i + 1].position == action.position;
            if (!opensDoubleClick) {
                controller.handleClick(action.position.x, action.position.y);
            }
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::vector<std::string> args(argv + 1, argv + argc);
        if (!args.empty() && args[0] == "--script") {
            return runScript(std::cin, std::cout);
        }

        // Parse an optional "--connect [host:port]" and an optional assets root.
        // Without --connect the game runs against a same-process loopback server;
        // with it, against a real one over a socket.
        std::optional<std::string> connectArg;
        std::string assetsRoot = "client/assets";
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--connect") {
                connectArg = "";  // default address unless an argument follows
                if (i + 1 < args.size() && args[i + 1].rfind("--", 0) != 0) {
                    connectArg = args[++i];
                }
            } else {
                assetsRoot = args[i];
            }
        }

        // Both play modes open from the same board file.
        std::ifstream boardFile{startPositionPath};
        if (!boardFile) {
            std::cout << "ERROR cannot open " << startPositionPath << "\n";
            return 1;
        }
        kfc::io::ParsedInput parsed = kfc::io::parseInput(boardFile);
        const int boardWidth = parsed.board.width();
        const int boardHeight = parsed.board.height();

        if (connectArg) {
            const std::optional<kfc::net::Endpoint> endpoint =
                kfc::net::parseEndpoint(*connectArg, defaultHost, defaultPort);
            if (!endpoint) {
                std::cout << "ERROR bad address " << *connectArg << "\n";
                return 1;
            }
            kfc::net::NetworkedGame game{endpoint->host, endpoint->port,
                                         parsed.board};
            return runGraphical(game, boardWidth, boardHeight, assetsRoot,
                                kfc::view::defaultCellPx);
        }

        // A LoopbackGame hosts the authoritative session and a client in this one
        // process, so local play travels the very same protocol path as a
        // networked game.
        kfc::net::LoopbackGame game{parsed.board};
        return runGraphical(game, boardWidth, boardHeight, assetsRoot,
                            kfc::view::defaultCellPx);
    } catch (const kfc::io::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 1;
    }
}
