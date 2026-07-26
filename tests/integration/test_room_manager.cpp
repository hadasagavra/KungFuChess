#include "third_party/doctest/doctest.h"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "server/app/include/room_manager.hpp"
#include "server/store/include/in_memory_user_store.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::io::CreateRoom;
using kfc::io::EnteredRoom;
using kfc::io::JoinRoom;
using kfc::io::Login;
using kfc::io::NoMatch;
using kfc::io::OpponentDisconnected;
using kfc::io::OpponentReconnected;
using kfc::io::PlayerCommand;
using kfc::io::RoleAssignment;
using kfc::io::RoomError;
using kfc::io::SeekGame;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::MoveEvent;
using kfc::model::Position;
using kfc::server::ClientId;
using kfc::server::InMemoryUserStore;
using kfc::server::MessageTransport;
using kfc::server::RoomManager;

namespace {

constexpr int boardHeight = 8;

Board twoKnights() {
    return kfc::io::buildBoard({". bN . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". wN . . . . . ."});
}

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

std::string login(const std::string& username) {
    return kfc::io::encode(Login{username, "pw"}, boardHeight);
}
std::string createRoom() { return kfc::io::encode(CreateRoom{}, boardHeight); }
std::string seekGame() { return kfc::io::encode(SeekGame{}, boardHeight); }
std::string joinRoom(const std::string& id) {
    return kfc::io::encode(JoinRoom{id}, boardHeight);
}
std::string command(Color player, Kind kind, Position from, Position to) {
    return kfc::io::encode(PlayerCommand{player, kind, from, to}, boardHeight);
}

// The room id a client was told it entered, if any.
std::optional<std::string> enteredRoom(const RecordingTransport& transport,
                                       ClientId client) {
    const auto found = transport.sent.find(client);
    if (found == transport.sent.end()) return std::nullopt;
    for (const std::string& message : found->second) {
        const auto decoded = kfc::io::decode(message, boardHeight);
        if (decoded && std::holds_alternative<EnteredRoom>(*decoded)) {
            return std::get<EnteredRoom>(*decoded).roomId;
        }
    }
    return std::nullopt;
}

std::optional<RoleAssignment> role(const RecordingTransport& transport,
                                   ClientId client) {
    const auto found = transport.sent.find(client);
    if (found == transport.sent.end()) return std::nullopt;
    for (const std::string& message : found->second) {
        const auto decoded = kfc::io::decode(message, boardHeight);
        if (decoded && std::holds_alternative<RoleAssignment>(*decoded)) {
            return std::get<RoleAssignment>(*decoded);
        }
    }
    return std::nullopt;
}

template <typename Message>
bool sentHas(const RecordingTransport& transport, ClientId client) {
    const auto found = transport.sent.find(client);
    if (found == transport.sent.end()) return false;
    for (const std::string& message : found->second) {
        const auto decoded = kfc::io::decode(message, boardHeight);
        if (decoded && std::holds_alternative<Message>(*decoded)) return true;
    }
    return false;
}

RoomManager makeManager(RecordingTransport& transport, InMemoryUserStore& users) {
    return RoomManager{transport, users, [] { return twoKnights(); }};
}

}  // namespace

TEST_CASE("creator is white, next joiner black, the rest spectators") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(2, login("Bob"));
    manager.handleMessage(3, login("Cara"));

    manager.handleMessage(1, createRoom());
    const std::optional<std::string> room = enteredRoom(transport, 1);
    REQUIRE(room);
    manager.handleMessage(2, joinRoom(*room));
    manager.handleMessage(3, joinRoom(*room));

    REQUIRE(role(transport, 1));
    CHECK(role(transport, 1)->color == Color::White);
    REQUIRE(role(transport, 2));
    CHECK(role(transport, 2)->color == Color::Black);
    REQUIRE(role(transport, 3));
    CHECK_FALSE(role(transport, 3)->color);  // spectator
}

TEST_CASE("joining an unknown room is refused") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(1, joinRoom("NOPE00"));

    CHECK(sentHas<RoomError>(transport, 1));
    CHECK_FALSE(enteredRoom(transport, 1));
}

TEST_CASE("a room action before logging in is refused") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, createRoom());  // no login first

    CHECK(sentHas<RoomError>(transport, 1));
    CHECK_FALSE(enteredRoom(transport, 1));
}

TEST_CASE("Play pairs two waiting seekers into one room") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    // Both fresh accounts start at the same rating, so they are within range.
    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(2, login("Bob"));

    manager.handleMessage(1, seekGame());  // queued
    manager.handleMessage(2, seekGame());  // matched with Alice

    const std::optional<std::string> roomOne = enteredRoom(transport, 1);
    const std::optional<std::string> roomTwo = enteredRoom(transport, 2);
    REQUIRE(roomOne);
    REQUIRE(roomTwo);
    CHECK(*roomOne == *roomTwo);  // same room
    REQUIRE(role(transport, 1));
    CHECK(role(transport, 1)->color == Color::White);  // the waiter takes white
    REQUIRE(role(transport, 2));
    CHECK(role(transport, 2)->color == Color::Black);
}

TEST_CASE("a lone Play search reports no match after the timeout") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(1, seekGame());
    manager.tick(60000);  // one minute passes with no opponent

    CHECK(sentHas<NoMatch>(transport, 1));
    CHECK_FALSE(enteredRoom(transport, 1));
}

TEST_CASE("a disconnect counts down and then forfeits the game") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(2, login("Bob"));
    manager.handleMessage(1, createRoom());  // Alice white
    manager.handleMessage(2, joinRoom(*enteredRoom(transport, 1)));  // Bob black

    manager.handleDisconnected(2);   // Bob drops
    manager.tick(1000);              // a second of the grace passes
    CHECK(sentHas<OpponentDisconnected>(transport, 1));  // Alice sees the countdown

    // Run out the 20-second grace: Bob forfeits, so Alice wins and gains rating.
    for (int i = 0; i < 25; ++i) manager.tick(1000);
    CHECK(users.authenticate("Alice", "pw").rating > 1200);
    CHECK(users.authenticate("Bob", "pw").rating < 1200);
}

TEST_CASE("reconnecting within the grace window cancels the forfeit") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    manager.handleMessage(1, login("Alice"));
    manager.handleMessage(2, login("Bob"));
    manager.handleMessage(1, createRoom());
    const std::string room = *enteredRoom(transport, 1);
    manager.handleMessage(2, joinRoom(room));  // Bob black

    manager.handleDisconnected(2);  // Bob drops
    manager.tick(1000);

    // Bob returns on a fresh connection (new id), logs in, rejoins the same room.
    manager.handleMessage(3, login("Bob"));
    manager.handleMessage(3, joinRoom(room));
    REQUIRE(role(transport, 3));
    CHECK(role(transport, 3)->color == Color::Black);  // same seat restored
    CHECK(sentHas<OpponentReconnected>(transport, 1));

    // The grace would have expired by now, but the seat was reclaimed: no forfeit.
    for (int i = 0; i < 25; ++i) manager.tick(1000);
    CHECK(users.authenticate("Alice", "pw").rating == 1200);
    CHECK(users.authenticate("Bob", "pw").rating == 1200);
}

TEST_CASE("two rooms are isolated") {
    RecordingTransport transport;
    InMemoryUserStore users;
    RoomManager manager = makeManager(transport, users);

    for (ClientId id = 1; id <= 4; ++id) {
        manager.handleMessage(id, login("player" + std::to_string(id)));
    }
    manager.handleMessage(1, createRoom());  // room A: white
    const std::string roomA = *enteredRoom(transport, 1);
    manager.handleMessage(2, joinRoom(roomA));  // room A: black
    manager.handleMessage(3, createRoom());     // room B: white
    manager.handleMessage(4, joinRoom(*enteredRoom(transport, 3)));  // room B

    // A move in room A is broadcast to room A only. (RoomTransport delivers
    // broadcasts as per-client sends, so they show up in `sent`, not `broadcasts`.)
    const Position b1{7, 1};
    const Position c3{5, 2};
    manager.handleMessage(1, command(Color::White, Kind::Knight, b1, c3));

    CHECK(sentHas<MoveEvent>(transport, 2));       // room A opponent sees it
    CHECK_FALSE(sentHas<MoveEvent>(transport, 3));  // room B players do not
    CHECK_FALSE(sentHas<MoveEvent>(transport, 4));
}
