#include "third_party/doctest/doctest.h"

#include <optional>
#include <string>

#include "client/net/include/remote_game.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/state_codec.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

using kfc::engine::GameSnapshot;
using kfc::io::AuthRejected;
using kfc::io::EnteredRoom;
using kfc::io::NoMatch;
using kfc::io::OpponentDisconnected;
using kfc::io::OpponentReconnected;
using kfc::io::PlayerRoster;
using kfc::io::RoleAssignment;
using kfc::io::RoomError;
using kfc::io::SeekGame;
using kfc::io::StateUpdate;
using kfc::io::WireMessage;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Position;
using kfc::net::RemoteGame;

namespace {

constexpr int boardHeight = 8;

// A white knight on b1 (row 7, col 1) and a black knight on b8 (row 0, col 1),
// each with legal moves onto the otherwise empty board.
Board twoKnights() {
    return kfc::io::buildBoard({". bN . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". . . . . . . .",
                                ". . . . . . . .", ". wN . . . . . ."});
}

const Position whiteKnight{7, 1};
const Position blackKnight{0, 1};

// A RemoteGame whose commands go nowhere: these tests only read what it draws.
RemoteGame makeRemote() {
    return RemoteGame{Board{boardHeight, boardHeight},
                      [](Color, const std::string&) {}};
}

std::string role(std::optional<Color> color) {
    return kfc::io::encode(WireMessage{RoleAssignment{color}}, boardHeight);
}

// A state frame carrying the two-knights position, so the replica holds real
// pieces the legal-move gate can reason about.
std::string knightsFrame() {
    Board board = twoKnights();
    return kfc::io::encode(
        WireMessage{StateUpdate{kfc::io::encodeState(GameSnapshot{board, false})}},
        boardHeight);
}

}  // namespace

TEST_CASE("a player highlights only its own colour") {
    RemoteGame remote = makeRemote();
    remote.receive(knightsFrame());
    remote.receive(role(Color::White));

    CHECK_FALSE(remote.legalDestinationsFor(whiteKnight).empty());
    CHECK(remote.legalDestinationsFor(blackKnight).empty());  // not my colour
}

TEST_CASE("a spectator highlights nothing") {
    RemoteGame remote = makeRemote();
    remote.receive(knightsFrame());
    remote.receive(role(std::nullopt));  // spectator: no colour

    CHECK(remote.legalDestinationsFor(whiteKnight).empty());
    CHECK(remote.legalDestinationsFor(blackKnight).empty());
}

TEST_CASE("a client seated as both colours highlights either") {
    // This is the loopback case: the one window is assigned white and then black,
    // so both role messages arrive and both colours may be highlighted.
    RemoteGame remote = makeRemote();
    remote.receive(knightsFrame());
    remote.receive(role(Color::White));
    remote.receive(role(Color::Black));

    CHECK_FALSE(remote.legalDestinationsFor(whiteKnight).empty());
    CHECK_FALSE(remote.legalDestinationsFor(blackKnight).empty());
}

TEST_CASE("a roster sets the player names and ratings") {
    RemoteGame remote = makeRemote();
    remote.receive(kfc::io::encode(
        WireMessage{PlayerRoster{"Alice", 1200, "Bob", 1284}}, boardHeight));

    CHECK(remote.whiteName() == "Alice");
    CHECK(remote.blackName() == "Bob");
    REQUIRE(remote.whiteRating());
    CHECK(*remote.whiteRating() == 1200);
    REQUIRE(remote.blackRating());
    CHECK(*remote.blackRating() == 1284);
}

TEST_CASE("a rejected login is recorded as an auth error") {
    RemoteGame remote = makeRemote();
    CHECK_FALSE(remote.authError());  // nothing rejected yet

    remote.receive(kfc::io::encode(
        WireMessage{AuthRejected{"incorrect password for Alice"}}, boardHeight));

    REQUIRE(remote.authError());
    CHECK(*remote.authError() == "incorrect password for Alice");
}

TEST_CASE("entering a room moves the client into the game") {
    RemoteGame remote = makeRemote();
    CHECK_FALSE(remote.isInGame());

    remote.receive(kfc::io::encode(WireMessage{EnteredRoom{"AB12CD"}}, boardHeight));

    CHECK(remote.isInGame());
    CHECK(remote.roomId() == "AB12CD");
    CHECK_FALSE(remote.isSearching());
}

TEST_CASE("seeking sets searching, and no-match reports it") {
    std::string sent;
    RemoteGame remote{Board{boardHeight, boardHeight},
                      [&](Color, const std::string& message) { sent = message; }};

    remote.seekGame();
    CHECK(remote.isSearching());
    CHECK_FALSE(sent.empty());  // a SeekGame went out

    remote.receive(kfc::io::encode(WireMessage{NoMatch{}}, boardHeight));
    CHECK_FALSE(remote.isSearching());
    CHECK(remote.statusMessage() == "Couldn't find a match");
}

TEST_CASE("a room error becomes the status message") {
    RemoteGame remote = makeRemote();
    remote.receive(kfc::io::encode(WireMessage{RoomError{"no room ZZ99"}},
                                   boardHeight));
    CHECK(remote.statusMessage() == "no room ZZ99");
}

TEST_CASE("an opponent's disconnect countdown is tracked and then cleared") {
    RemoteGame remote = makeRemote();
    CHECK_FALSE(remote.opponentCountdown());

    remote.receive(
        kfc::io::encode(WireMessage{OpponentDisconnected{12}}, boardHeight));
    REQUIRE(remote.opponentCountdown());
    CHECK(*remote.opponentCountdown() == 12);

    remote.receive(kfc::io::encode(WireMessage{OpponentReconnected{}}, boardHeight));
    CHECK_FALSE(remote.opponentCountdown());
}
