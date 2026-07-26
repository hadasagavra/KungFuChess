#include "third_party/doctest/doctest.h"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "server/app/include/game_session.hpp"
#include "server/store/include/in_memory_user_store.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/command_notation.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::AuthRejected;
using kfc::io::Login;
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
using kfc::server::InMemoryUserStore;
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

std::string login(const std::string& username,
                  const std::string& password = "pw") {
    return kfc::io::encode(Login{username, password}, boardHeight);
}

// The last roster the session broadcast, if it broadcast one at all.
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
    InMemoryUserStore users;
    GameSession session{board, transport, users};

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
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white

    session.handleMessage(1, command(Color::White, Kind::Knight, b1, c3));

    // The engine accepted it, so it published a MoveEvent the session relayed.
    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a command for a colour the client does not play is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white

    // The white client tries to order a black move: authorization, not legality.
    session.handleMessage(1, command(Color::Black, Kind::Knight,
                                     Position{0, 1}, Position{2, 2}));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a spectator's command is refused") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);
    session.addClient(2);
    session.addClient(3);  // spectator

    session.handleMessage(3, command(Color::White, Kind::Knight, b1, c3));

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a jump command is accepted as a jump") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white

    // from == to: the knight jumps in place. A jump has no legality gate beyond
    // being idle, so the engine accepts it and relays the event.
    session.handleMessage(1, command(Color::White, Kind::Knight, b1, b1));

    CHECK(broadcastHas<MoveEvent>(transport));
}

TEST_CASE("a tick broadcasts a state frame") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);

    session.tick(16);

    CHECK(broadcastHas<StateUpdate>(transport));
}

TEST_CASE("a login broadcasts a roster naming that colour") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white

    session.handleMessage(1, login("Alice"));

    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteName);
    CHECK(*roster->whiteName == "Alice");
    REQUIRE(roster->whiteRating);
    CHECK(*roster->whiteRating == 1200);  // a fresh account starts here
    CHECK_FALSE(roster->blackName);  // black seat empty / not logged in
}

TEST_CASE("both logins name both seats in the roster") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white
    session.addClient(2);  // black

    session.handleMessage(1, login("Alice"));
    session.handleMessage(2, login("Bob"));

    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteName);
    CHECK(*roster->whiteName == "Alice");
    REQUIRE(roster->blackName);
    CHECK(*roster->blackName == "Bob");
}

TEST_CASE("a spectator's login names no seat") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white
    session.addClient(2);  // black
    session.addClient(3);  // spectator

    session.handleMessage(1, login("Alice"));
    session.handleMessage(2, login("Bob"));
    session.handleMessage(3, login("Cara"));  // a spectator, seated nowhere

    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteName);
    CHECK(*roster->whiteName == "Alice");
    REQUIRE(roster->blackName);
    CHECK(*roster->blackName == "Bob");  // Cara appears on neither seat
}

TEST_CASE("a login with the wrong password is rejected") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white
    session.addClient(2);  // black

    session.handleMessage(1, login("Alice", "secret"));  // registers Alice
    session.handleMessage(2, login("Alice", "wrong"));   // same name, bad password

    // The second client was told, personally, that its login was refused.
    REQUIRE_FALSE(transport.sent[2].empty());
    const std::optional<kfc::io::WireMessage> last =
        kfc::io::decode(transport.sent[2].back(), boardHeight);
    REQUIRE(last);
    CHECK(std::holds_alternative<AuthRejected>(*last));
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
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);  // white
    session.addClient(2);  // black
    session.handleMessage(1, login("Alice", "pw"));  // both start at 1200
    session.handleMessage(2, login("Bob", "pw"));

    session.handleMessage(1, command(Color::White, Kind::Rook, a1, a2));
    // Let the slide arrive; the capture resolves within a second of game time.
    for (int i = 0; i < 30 && !broadcastHas<CapturedPiece>(transport); ++i) {
        session.tick(100);
    }
    REQUIRE(broadcastHas<CapturedPiece>(transport));

    const std::optional<PlayerRoster> roster = lastRoster(transport);
    REQUIRE(roster);
    REQUIRE(roster->whiteRating);
    REQUIRE(roster->blackRating);
    CHECK(*roster->whiteRating > 1200);  // the winner gained
    CHECK(*roster->blackRating < 1200);  // the loser lost
}

TEST_CASE("malformed text from a client is ignored") {
    Board board = twoKnights();
    RecordingTransport transport;
    InMemoryUserStore users;
    GameSession session{board, transport, users};
    session.addClient(1);

    session.handleMessage(1, "this is not a message");

    CHECK_FALSE(broadcastHas<MoveEvent>(transport));
    CHECK_FALSE(broadcastHas<StateUpdate>(transport));
}
