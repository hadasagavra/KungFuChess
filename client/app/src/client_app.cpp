#include "client/app/include/client_app.hpp"

#include <cstdint>
#include <fstream>
#include <optional>

#include "client/app/include/cli.hpp"
#include "client/app/include/game_app.hpp"
#include "client/app/include/script_mode.hpp"
#include "client/net/include/endpoint.hpp"
#include "client/net/include/loopback_game.hpp"
#include "client/net/include/networked_game.hpp"
#include "client/view/include/render_config.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/model/include/board.hpp"

namespace kfc::app {
namespace {

// Where the opening position is read from -- the same file the server loads, so
// one description of the board serves both composition roots.
const char* const startPositionPath = "config/start_position.txt";

// The default server address for --connect when it is given no address.
const char* const defaultHost = "127.0.0.1";
constexpr std::uint16_t defaultPort = 9000;

}  // namespace

int runClient(const std::vector<std::string>& args, std::istream& in,
              std::ostream& out) {
    try {
        const ClientOptions options = parseClientOptions(args);
        if (options.script) return runScript(in, out);

        std::ifstream boardFile{startPositionPath};
        if (!boardFile) {
            out << "ERROR cannot open " << startPositionPath << "\n";
            return 1;
        }
        kfc::io::ParsedInput parsed = kfc::io::parseInput(boardFile);
        const int boardWidth = parsed.board.width();
        const int boardHeight = parsed.board.height();

        if (options.connect) {
            const std::optional<net::Endpoint> endpoint =
                net::parseEndpoint(*options.connect, defaultHost, defaultPort);
            if (!endpoint) {
                out << "ERROR bad address " << *options.connect << "\n";
                return 1;
            }
            // Ask for credentials on the shell before opening the window, and
            // carry them to the server so it can authenticate and name the seats.
            const Credentials credentials = promptCredentials(in, out);
            net::NetworkedGame game{endpoint->host, endpoint->port, parsed.board,
                                    credentials.username, credentials.password};
            GameApp app{game, boardWidth, boardHeight, options.assetsRoot,
                        view::defaultCellPx};
            return app.run();
        }

        // A LoopbackGame hosts the authoritative session and a client in this one
        // process, so local play travels the very same protocol path as a
        // networked game.
        net::LoopbackGame game{parsed.board};
        GameApp app{game, boardWidth, boardHeight, options.assetsRoot,
                    view::defaultCellPx};
        return app.run();
    } catch (const kfc::io::ParseError& e) {
        out << "ERROR " << e.code << "\n";
        return 1;
    }
}

}  // namespace kfc::app
