#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "server/app/include/room_manager.hpp"
#include "server/net/include/websocket_server.hpp"
#include "server/store/include/sqlite_user_store.hpp"
#include "shared/log/include/logger.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/model/include/board.hpp"

namespace {

// Where the starting board lives, resolved from the working directory -- the same
// text the client loads, so the two composition roots share one description of
// the opening position instead of each hard-coding it.
const char* const startPositionPath = "config/start_position.txt";

// The account database, created beside the server on first run.
const char* const userDatabasePath = "kfc_users.db";

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

// Read the whole opening-position file once; each room is built from a fresh
// parse of this text, so no two rooms share pieces.
std::string readFile(const std::string& path) {
    std::ifstream file{path};
    if (!file) throw std::runtime_error("cannot open " + path);
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

}  // namespace

// The server composition root: host the lobby over a WebSocket and drive the
// clock. No window, no sprites, no OpenCV -- the Server layer needs none of the
// GUI, and this is where that shows.
int main(int argc, char** argv) {
    try {
        const std::string boardText = readFile(startPositionPath);
        // A fresh board per room: rooms must never share pieces, and a Board copy
        // would (it holds shared_ptrs), so each is re-parsed from the text.
        const auto boardFactory = [boardText]() {
            std::istringstream in{boardText};
            return kfc::io::parseInput(in).board;
        };

        const std::uint16_t port = portFrom(argc, argv);
        kfc::server::WebSocketServer transport{port};
        kfc::server::SqliteUserStore users{userDatabasePath};
        kfc::server::RoomManager manager{transport, users, boardFactory};

        transport.onClientConnected([](kfc::server::ClientId id) {
            kfc::log::write("server", "client " + std::to_string(id) + " connected");
        });
        transport.onMessage(
            [&manager](kfc::server::ClientId id, const std::string& message) {
                manager.handleMessage(id, message);
            });
        transport.onClientDisconnected([&manager](kfc::server::ClientId id) {
            kfc::log::write("server",
                            "client " + std::to_string(id) + " disconnected");
            manager.handleDisconnected(id);
        });

        kfc::log::write("server", "listening on port " + std::to_string(port));
        std::cout << "KungFuChess server listening on port " << port << "\n";

        auto last = std::chrono::steady_clock::now();
        while (true) {
            transport.poll();  // accept connections, read incoming messages
            const auto now = std::chrono::steady_clock::now();
            manager.tick(elapsedMsSince(last, now));
            last = now;
            std::this_thread::sleep_for(std::chrono::milliseconds(frameMs));
        }
    } catch (const kfc::io::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cout << "ERROR " << e.what() << "\n";
        return 1;
    }
}
