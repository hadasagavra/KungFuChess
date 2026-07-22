#include "third_party/doctest/doctest.h"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "server/app/include/game_session.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/command_notation.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::PlayerCommand;
using kfc::io::RoleAssignment;
using kfc::io::StateUpdate;
using kfc::model::Board;
using kfc::model::CapturedPiece;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::model::Position;
using kfc::server::ClientId;
using kfc::server::GameSession;
using kfc::server::MessageTransport;

namespace {

constexpr int boardHeight = 8;

// A white knight on b1 (row 7, col 1) that may legally reach the empty c3
// (row 5, col 2); a black knight on b8 so the black colour is a real player too.
Board twoKnights() {
    return kfc::io::buildBoard({". bN . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". wN . . . . . ."});
}

const Position b1{7, 1};
const Position c3{5, 2};

// Everything the session sent, so a test can read back what each client was
// told and what was broadcast to all.
class RecordingTransport : public MessageTransport {
public:
    void send(ClientId client, const std::string& message) override {
        sent[client].push_back(message);
    }
    void broadcast(const std::string& message) override {
        broadcasts.push_back(message);
    }

    std::map<ClientId, std::vector<std::string>> sent;
    std::vector<std::string> broadcasts;
};

std::string command(Color player, Kind kind, Position from, Position to) {
    return kfc::io::encode(PlayerCommand{player, kind, from, to}, boardHeight);
}

// Whether any broadcast decodes to a message of the given alternative.
template <typename Message>
bool broadcastHas(const RecordingTransport& transport) {
    for (const std::string& message : transport.broadcasts) {
        const std::optional<kfc::io::WireMessage> decoded =
            kfc::io::decode(message, boardHeight);
        if (decoded && std::holds_alternative<Message>(*decoded)) return true;
    }
    return false;
}

std::optional<RoleAssignment> lastRole(const RecordingTransport& transport,
                                       ClientId client) {
    const auto found = transport.sent.find(client);
    if (found == transport.sent.end() || found->second.empty()) {
        return std::nullopt;
    }
    const std::optional<kfc::io::WireMessage> decoded =
        kfc::io::decode(found->second.back(), boardHeight);
    if (decoded && std::holds_alternative<RoleAssignment>(*decoded)) {
        return std::get<RoleAssignment>(*decoded);
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("clients are seated white, then black, then spectator") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};

    session.addClient(1);
    session.addClient(2);
    session.addClient(3);

    REQUIRE(lastRole(transport, 1));
    CHECK(lastRole(transport, 1)->color == Color::White);
    REQUIRE(lastRole(transport, 2));
    CHECK(lastRole(transport, 2)->color == Color::Black);
    REQUIRE(lastRole(transport, 3));
    CHECK_FALSE(lastRole(transport, 3)->color);  // spectator
}

TEST_CASE("a command from the owning colour reaches the engine") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);  // white

    session.handleMessage(1, command(Color::White, Kind::Knight, b1, c3));

    // The engine accepted it, so it published a MoveEvent the session relayed.
    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a command for a colour the client does not play is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);  // white

    // The white client tries to order a black move: authorization, not legality.
    session.handleMessage(1, command(Color::Black, Kind::Knight,
                                     Position{0, 1}, Position{2, 2}));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a spectator's command is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);
    session.addClient(2);
    session.addClient(3);  // spectator

    session.handleMessage(3, command(Color::White, Kind::Knight, b1, c3));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a jump command is accepted as a jump") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);  // white

    // from == to: the knight jumps in place. A jump has no legality gate beyond
    // being idle, so the engine accepts it and relays the event.
    session.handleMessage(1, command(Color::White, Kind::Knight, b1, b1));

    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a tick broadcasts a state frame") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);

    session.tick(16);

    CHECK(broadcastHas<StateUpdate>(transport));
}

TEST_CASE("malformed text from a client is ignored") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1);

    session.handleMessage(1, "this is not a message");

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
    CHECK_FALSE(broadcastHas<StateUpdate>(transport));
}
