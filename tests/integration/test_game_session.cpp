#include "third_party/doctest/doctest.h"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "server/app/include/game_session.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::PlayerCommand;
using kfc::io::PlayerRoster;
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

// Everything the session sent, so a test can read back what each client was told
// and what was broadcast to all.
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

std::optional<PlayerRoster> lastRoster(const RecordingTransport& transport) {
    std::optional<PlayerRoster> found;
    for (const std::string& message : transport.broadcasts) {
        const std::optional<kfc::io::WireMessage> decoded =
            kfc::io::decode(message, boardHeight);
        if (decoded && std::holds_alternative<PlayerRoster>(*decoded)) {
            found = std::get<PlayerRoster>(*decoded);
        }
    }
    return found;
}

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
    for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
        const std::optional<kfc::io::WireMessage> decoded =
            kfc::io::decode(*it, boardHeight);
        if (decoded && std::holds_alternative<RoleAssignment>(*decoded)) {
            return std::get<RoleAssignment>(*decoded);
        }
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("clients are seated white, then black, then spectator") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};

    session.addClient(1, "Alice", 1200);
    session.addClient(2, "Bob", 1200);
    session.addClient(3, "Cara", 1200);

    REQUIRE(lastRole(transport, 1));
    CHECK(lastRole(transport, 1)->color == Color::White);
    REQUIRE(lastRole(transport, 2));
    CHECK(lastRole(transport, 2)->color == Color::Black);
    REQUIRE(lastRole(transport, 3));
    CHECK_FALSE(lastRole(transport, 3)->color);  // spectator
}

TEST_CASE("seating publishes a roster with names and ratings") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};

    session.addClient(1, "Alice", 1200);
    session.addClient(2, "Bob", 1284);

    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteName);
    CHECK(*roster->whiteName == "Alice");
    REQUIRE(roster->whiteRating);
    CHECK(*roster->whiteRating == 1200);
    REQUIRE(roster->blackName);
    CHECK(*roster->blackName == "Bob");
    REQUIRE(roster->blackRating);
    CHECK(*roster->blackRating == 1284);
}

TEST_CASE("a command from the owning colour reaches the engine") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);  // white

    session.handleMessage(1, command(Color::White, Kind::Knight, b1, c3));

    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a command for a colour the client does not play is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);  // white

    session.handleMessage(1, command(Color::Black, Kind::Knight, Position{0, 1},
                                     Position{2, 2}));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a spectator's command is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);
    session.addClient(2, "Bob", 1200);
    session.addClient(3, "Cara", 1200);  // spectator

    session.handleMessage(3, command(Color::White, Kind::Knight, b1, c3));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a jump command is accepted as a jump") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);  // white

    session.handleMessage(1, command(Color::White, Kind::Knight, b1, b1));

    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a tick broadcasts a state frame") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);

    session.tick(16);

    CHECK(broadcastHas<StateUpdate>(transport));
}

TEST_CASE("capturing the king settles both players' ratings by Elo") {
    // Black king on a2 (row 6), white rook on a1 (row 7): the rook steps up one
    // square to take the king, which ends the game.
    Board board = kfc::io::buildBoard({
        ". . . . . . . .", ". . . . . . . .", ". . . . . . . .",
        ". . . . . . . .", ". . . . . . . .", ". . . . . . . .",
        "bK . . . . . . .", "wR . . . . . . ."});
    const Position a1{7, 0};
    const Position a2{6, 0};

    RecordingTransport transport;
    std::map<std::string, int> persisted;
    GameSession session{board, transport,
                        [&](const std::string& name, int rating) {
                            persisted[name] = rating;
                        }};
    session.addClient(1, "Alice", 1200);  // white
    session.addClient(2, "Bob", 1200);    // black

    session.handleMessage(1, command(Color::White, Kind::Rook, a1, a2));
    for (int i = 0; i < 30 && !broadcastHas<CapturedPiece>(transport); ++i) {
        session.tick(100);
    }
    REQUIRE(broadcastHas<CapturedPiece>(transport));

    // The winner's new rating was persisted, and the roster shows the change.
    CHECK(persisted["Alice"] > 1200);
    CHECK(persisted["Bob"] < 1200);
    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteRating);
    CHECK(*roster->whiteRating > 1200);
    REQUIRE(roster->blackRating);
    CHECK(*roster->blackRating < 1200);
}

TEST_CASE("a forfeit ends the game and settles ratings") {
    Board board = twoKnights();
    RecordingTransport transport;
    std::map<std::string, int> persisted;
    GameSession session{board, transport,
                        [&](const std::string& name, int rating) {
                            persisted[name] = rating;
                        }};
    session.addClient(1, "Alice", 1200);  // white
    session.addClient(2, "Bob", 1200);    // black

    session.forfeit(Color::Black);  // black dropped out

    CHECK(session.isOver());
    CHECK(persisted["Alice"] > 1200);  // white wins the forfeit
    CHECK(persisted["Bob"] < 1200);
}

TEST_CASE("malformed text from a client is ignored") {
    Board board = twoKnights();
    RecordingTransport transport;
    GameSession session{board, transport};
    session.addClient(1, "Alice", 1200);

    session.handleMessage(1, "this is not a message");

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}
