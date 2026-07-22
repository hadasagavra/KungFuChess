#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "server/app/include/game_session.hpp"
#include "server/net/include/websocket_server.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/model/include/board.hpp"

namespace {

// Where the starting board lives, resolved from the working directory -- the same
// text the client loads, so the two composition roots share one description of
// the opening position instead of each hard-coding it.
const char* const startPositionPath = "config/start_position.txt";

// The default listen port; a first argument overrides it.
constexpr std::uint16_t defaultPort = 9000;

// One server frame. Small enough that the real-time clock stays smooth, and the
// same clamp the client uses so a stalled frame cannot jump the clock.
constexpr int frameMs = 16;
constexpr int maxStepMs = 100;

int elapsedMsSince(std::chrono::steady_clock::time_point last,
                   std::chrono::steady_clock::time_point now) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    return std::clamp(static_cast<int>(ms), 0, maxStepMs);
}

std::uint16_t portFrom(int argc, char** argv) {
    if (argc > 1) {
        return static_cast<std::uint16_t>(std::stoi(argv[1]));
    }
    return defaultPort;
}

}  // namespace

// The server composition root: build the authoritative board, host it over a
// WebSocket, and drive the clock. No window, no sprites, no OpenCV -- the Server
// layer needs none of the GUI, and this is where that shows.
int main(int argc, char** argv) {
    std::ifstream boardFile{startPositionPath};
    if (!boardFile) {
        std::cout << "ERROR cannot open " << startPositionPath << "\n";
        return 1;
    }

    try {
        kfc::io::ParsedInput parsed = kfc::io::parseInput(boardFile);
        kfc::model::Board board = std::move(parsed.board);

        const std::uint16_t port = portFrom(argc, argv);
        kfc::server::WebSocketServer transport{port};
        kfc::server::GameSession session{board, transport};

        transport.onClientConnected(
            [&session](kfc::server::ClientId id) { session.addClient(id); });
        transport.onMessage(
            [&session](kfc::server::ClientId id, const std::string& message) {
                session.handleMessage(id, message);
            });

        std::cout << "KungFuChess server listening on port " << port << "\n";

        auto last = std::chrono::steady_clock::now();
        while (true) {
            transport.poll();  // accept connections, read incoming commands
            const auto now = std::chrono::steady_clock::now();
            session.tick(elapsedMsSince(last, now));
            last = now;
            std::this_thread::sleep_for(std::chrono::milliseconds(frameMs));
        }
    } catch (const kfc::io::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 1;
    }
}
