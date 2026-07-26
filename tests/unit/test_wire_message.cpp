#include "third_party/doctest/doctest.h"

#include <optional>
#include <string>
#include <variant>

#include "shared/logic/io/include/wire_message.hpp"

using kfc::io::AuthRejected;
using kfc::io::CancelSeek;
using kfc::io::CreateRoom;
using kfc::io::decode;
using kfc::io::encode;
using kfc::io::EnteredRoom;
using kfc::io::JoinRoom;
using kfc::io::Login;
using kfc::io::NoMatch;
using kfc::io::OpponentDisconnected;
using kfc::io::OpponentReconnected;
using kfc::io::PlayerRoster;
using kfc::io::RoomError;
using kfc::io::SeekGame;
using kfc::io::WireMessage;

namespace {

// Login and roster carry no board coordinates, so the board height is irrelevant
// to them; any value round-trips the same.
constexpr int boardHeight = 8;

template <typename Message>
std::optional<Message> roundTrip(const Message& message) {
    const std::optional<WireMessage> decoded =
        decode(encode(WireMessage{message}, boardHeight), boardHeight);
    if (decoded && std::holds_alternative<Message>(*decoded)) {
        return std::get<Message>(*decoded);
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("a login round-trips its username and password") {
    const std::optional<Login> back = roundTrip(Login{"Alice", "s3cret"});
    REQUIRE(back);
    CHECK(back->username == "Alice");
    CHECK(back->password == "s3cret");
}

TEST_CASE("a username or password with spaces survives the round-trip") {
    // Each field is the remainder of its own line, so inner spaces are kept.
    const std::optional<Login> back =
        roundTrip(Login{"Alice the Great", "correct horse battery"});
    REQUIRE(back);
    CHECK(back->username == "Alice the Great");
    CHECK(back->password == "correct horse battery");
}

TEST_CASE("an incomplete login is not a message") {
    // The encoder never emits one, but a peer might: a blank field or a missing
    // one cannot be authenticated, so it decodes to nothing.
    CHECK_FALSE(decode("LOGIN user Alice\n", boardHeight));         // no password
    CHECK_FALSE(decode("LOGIN user Alice\npass \n", boardHeight));  // blank password
    CHECK_FALSE(decode("LOGIN pass secret\n", boardHeight));        // no username
}

TEST_CASE("a roster round-trips both seat names and ratings") {
    const std::optional<PlayerRoster> back =
        roundTrip(PlayerRoster{"Alice", 1200, "Bob", 1284});
    REQUIRE(back);
    REQUIRE(back->whiteName);
    CHECK(*back->whiteName == "Alice");
    REQUIRE(back->whiteRating);
    CHECK(*back->whiteRating == 1200);
    REQUIRE(back->blackName);
    CHECK(*back->blackName == "Bob");
    REQUIRE(back->blackRating);
    CHECK(*back->blackRating == 1284);
}

TEST_CASE("a roster with only one seat named keeps the other empty") {
    const std::optional<PlayerRoster> whiteOnly = roundTrip(
        PlayerRoster{"Alice", 1200, std::nullopt, std::nullopt});
    REQUIRE(whiteOnly);
    REQUIRE(whiteOnly->whiteName);
    CHECK(*whiteOnly->whiteName == "Alice");
    REQUIRE(whiteOnly->whiteRating);
    CHECK(*whiteOnly->whiteRating == 1200);
    CHECK_FALSE(whiteOnly->blackName);
    CHECK_FALSE(whiteOnly->blackRating);
}

TEST_CASE("a seat name with spaces survives alongside its rating") {
    const std::optional<PlayerRoster> back = roundTrip(
        PlayerRoster{"Alice the Great", 1350, std::nullopt, std::nullopt});
    REQUIRE(back);
    REQUIRE(back->whiteName);
    CHECK(*back->whiteName == "Alice the Great");
    REQUIRE(back->whiteRating);
    CHECK(*back->whiteRating == 1350);
}

TEST_CASE("an empty roster round-trips to no names") {
    const std::optional<PlayerRoster> back = roundTrip(
        PlayerRoster{std::nullopt, std::nullopt, std::nullopt, std::nullopt});
    REQUIRE(back);
    CHECK_FALSE(back->whiteName);
    CHECK_FALSE(back->blackName);
}

TEST_CASE("an auth rejection round-trips its reason") {
    const std::optional<AuthRejected> back =
        roundTrip(AuthRejected{"incorrect password for Alice"});
    REQUIRE(back);
    CHECK(back->reason == "incorrect password for Alice");
}

TEST_CASE("the empty lobby messages round-trip by type") {
    CHECK(roundTrip(SeekGame{}));
    CHECK(roundTrip(CancelSeek{}));
    CHECK(roundTrip(CreateRoom{}));
    CHECK(roundTrip(NoMatch{}));
    CHECK(roundTrip(OpponentReconnected{}));
}

TEST_CASE("a join carries its room id") {
    const std::optional<JoinRoom> back = roundTrip(JoinRoom{"AB12CD"});
    REQUIRE(back);
    CHECK(back->roomId == "AB12CD");
    // A blank room id is meaningless, so it is not a message.
    CHECK_FALSE(decode("JOIN ", boardHeight));
}

TEST_CASE("entering a room carries the room id") {
    const std::optional<EnteredRoom> back = roundTrip(EnteredRoom{"AB12CD"});
    REQUIRE(back);
    CHECK(back->roomId == "AB12CD");
}

TEST_CASE("a room error carries its reason") {
    const std::optional<RoomError> back = roundTrip(RoomError{"no such room"});
    REQUIRE(back);
    CHECK(back->reason == "no such room");
}

TEST_CASE("a disconnect countdown carries the seconds left") {
    const std::optional<OpponentDisconnected> back =
        roundTrip(OpponentDisconnected{17});
    REQUIRE(back);
    CHECK(back->secondsLeft == 17);
    // A non-numeric countdown is malformed.
    CHECK_FALSE(decode("OPPGONE soon", boardHeight));
}
