#include "third_party/doctest/doctest.h"

#include <chrono>
#include <functional>
#include <thread>

#include "client/net/include/networked_game.hpp"
#include "server/app/include/game_session.hpp"
#include "server/net/include/websocket_server.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

// A real socket test, deliberately kept in its own binary (socket_tests) so its
// timing and its need to bind a localhost port never colour the deterministic
// unit suite. It proves the one thing unit tests cannot: that the same code the
// app runs actually carries a command out over a WebSocket and a state frame
// back, with no loopback shortcut.

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Position;
using kfc::net::NetworkedGame;
using kfc::server::ClientId;
using kfc::server::GameSession;
using kfc::server::WebSocketServer;

namespace {

// A port unlikely to clash with a real service; the server sets SO_REUSEADDR.
constexpr std::uint16_t testPort = 34567;

const Position b1{7, 1};
const Position c3{5, 2};

Board twoKnights() {
    return kfc::io::buildBoard({". bN . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". wN . . . . . ."});
}

}  // namespace

TEST_CASE("a command crosses a real websocket and the state comes back") {
    Board board = twoKnights();
    WebSocketServer transport{testPort};
    // This test drives a GameSession directly (not through the lobby): it seats
    // the connecting client itself, so the client's own login is simply ignored.
    GameSession session{board, transport};
    transport.onClientConnected(
        [&session](ClientId id) { session.addClient(id, "Tester", 1200); });
    transport.onMessage([&session](ClientId id, const std::string& message) {
        session.handleMessage(id, message);
    });

    // The client starts from an empty board of the right size, so a populated
    // square is proof a state frame actually arrived over the socket rather than
    // being assumed. (The real app seeds the replica from the config board, so it
    // is never undersized; here empty-but-sized makes the arrival observable.)
    NetworkedGame game{"127.0.0.1", testPort, Board{8, 8}, "Tester", "pw"};

    // Pump both ends: the server accepts and reads, the clock advances and
    // broadcasts, the client pumps its socket. Stop as soon as `done` holds, or
    // give up after the cap so a broken wire fails instead of hanging.
    const auto pump = [&](int stepMs, int maxIterations,
                          const std::function<bool()>& done) {
        for (int i = 0; i < maxIterations; ++i) {
            if (done()) return true;
            transport.poll();
            session.tick(stepMs);
            game.advance(0);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return done();
    };

    // The handshake completes and the first state frame lands: the replica fills.
    const bool connected =
        pump(20, 400, [&] { return game.pieceAt(b1).has_value(); });
    REQUIRE(connected);
    CHECK(game.pieceAt(b1)->getColor() == Color::White);
    CHECK(game.pieceAt(b1)->getKind() == Kind::Knight);

    // Send a move; it must travel out, be run by the server's engine, and the
    // resulting state must travel back and land on the replica.
    game.requestMove(b1, c3);
    const bool arrived =
        pump(100, 200, [&] { return game.pieceAt(c3).has_value(); });

    CHECK(arrived);
    CHECK(game.pieceAt(c3));
    CHECK_FALSE(game.pieceAt(b1));
}
