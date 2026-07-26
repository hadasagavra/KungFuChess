#include "shared/logic/io/include/wire_message.hpp"

#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include "shared/logic/io/include/event_codec.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/io/include/text.hpp"

namespace kfc::io {
namespace {

// The tag that names each message kind on the wire. Confined to this file: it is
// the private alphabet of the protocol, and nothing outside encode/decode reads
// or writes it.
constexpr const char* commandTag = "CMD";      // a client's move/jump request
constexpr const char* roleTag = "ROLE";        // the colour a client plays
constexpr const char* moveTag = "MOVE";        // a MoveEvent
constexpr const char* captureTag = "CAPTURE";  // a CapturedPiece
constexpr const char* stateTag = "STATE";      // a whole game frame
constexpr const char* loginTag = "LOGIN";      // a client's credentials
constexpr const char* rosterTag = "ROSTER";    // both players' names + ratings
constexpr const char* authFailTag = "AUTHFAIL";  // a refused login

// Lobby message tags.
constexpr const char* seekTag = "SEEK";        // Play: find any opponent
constexpr const char* cancelSeekTag = "CANCEL";  // stop looking
constexpr const char* createRoomTag = "CREATE";  // open a new room
constexpr const char* joinRoomTag = "JOIN";      // join a room by id
constexpr const char* enteredRoomTag = "ROOM";   // you are in this room
constexpr const char* noMatchTag = "NOMATCH";    // search gave up
constexpr const char* roomErrorTag = "ROOMERR";  // a lobby request failed
constexpr const char* oppGoneTag = "OPPGONE";    // opponent dropped, N seconds
constexpr const char* oppBackTag = "OPPBACK";    // opponent returned

// The role payload for a spectator, distinct from either colour letter.
constexpr char spectatorMark = '-';

// Inside a login payload each field is one line: this keyword, a space, then the
// value (which may itself hold spaces).
constexpr const char* userKeyword = "user";
constexpr const char* passKeyword = "pass";

// Inside a roster payload each named seat is one line: this keyword, a space,
// the rating, a space, then the name (which may itself hold spaces). A seat with
// no name is omitted.
constexpr const char* whiteSeatKeyword = "white";
constexpr const char* blackSeatKeyword = "black";

// A tag joined to its payload. The payload may hold spaces and newlines (a state
// frame does); only the first space, the one this inserts, separates the two.
std::string join(const std::string& tag, const std::string& payload) {
    return tag + " " + payload;
}

// A message split at its first space into the tag and the payload after it.
struct Split {
    std::string tag;
    std::string payload;
};

Split split(const std::string& message) {
    const std::size_t space = message.find(' ');
    if (space == std::string::npos) return {message, ""};
    return {message.substr(0, space), message.substr(space + 1)};
}

std::string encodeRole(const RoleAssignment& role) {
    const char letter = role.color ? colorLetter(*role.color) : spectatorMark;
    return join(roleTag, std::string(1, letter));
}

std::optional<RoleAssignment> decodeRole(const std::string& payload) {
    if (payload.size() != 1) return std::nullopt;
    const char letter = payload[0];
    if (letter == colorLetter(model::Color::White)) {
        return RoleAssignment{model::Color::White};
    }
    if (letter == colorLetter(model::Color::Black)) {
        return RoleAssignment{model::Color::Black};
    }
    if (letter == spectatorMark) return RoleAssignment{std::nullopt};
    return std::nullopt;
}

// One "keyword value" line, the value being the whole remainder so it may hold
// spaces. Split at the first space; nullopt if the line has no keyword+value.
struct KeyValue {
    std::string keyword;
    std::string value;
};

std::optional<KeyValue> keyValue(const std::string& line) {
    const std::size_t space = line.find(' ');
    if (space == std::string::npos) return std::nullopt;
    return KeyValue{line.substr(0, space), line.substr(space + 1)};
}

std::string encodeLogin(const Login& login) {
    const std::string payload = std::string(userKeyword) + ' ' + login.username +
                                '\n' + passKeyword + ' ' + login.password + '\n';
    return join(loginTag, payload);
}

std::optional<Login> decodeLogin(const std::string& payload) {
    std::optional<std::string> username;
    std::optional<std::string> password;
    std::istringstream in{payload};
    std::string line;
    while (std::getline(in, line)) {
        const std::optional<KeyValue> field = keyValue(line);
        if (!field) continue;
        if (field->keyword == userKeyword) username = field->value;
        else if (field->keyword == passKeyword) password = field->value;
    }
    // Both fields are required and neither may be blank: an incomplete login
    // cannot be authenticated, so it is not a message.
    if (!username || !password) return std::nullopt;
    if (trim(*username).empty() || trim(*password).empty()) return std::nullopt;
    return Login{*username, *password};
}

// One roster line: the seat keyword, the rating, then the name. The rating is a
// single token; the name is the remainder, so it may hold spaces of its own.
std::string rosterLine(const char* seatKeyword, int rating,
                       const std::string& name) {
    return std::string(seatKeyword) + ' ' + std::to_string(rating) + ' ' + name +
           '\n';
}

std::string encodeRoster(const PlayerRoster& roster) {
    std::string payload;
    if (roster.whiteName) {
        payload += rosterLine(whiteSeatKeyword, roster.whiteRating.value_or(0),
                              *roster.whiteName);
    }
    if (roster.blackName) {
        payload += rosterLine(blackSeatKeyword, roster.blackRating.value_or(0),
                              *roster.blackName);
    }
    return join(rosterTag, payload);
}

std::optional<PlayerRoster> decodeRoster(const std::string& payload) {
    PlayerRoster roster;
    std::istringstream in{payload};
    std::string line;
    while (std::getline(in, line)) {
        const std::optional<KeyValue> field = keyValue(line);
        if (!field) continue;  // no rating/name on this line
        const std::optional<KeyValue> rest = keyValue(field->value);
        if (!rest) continue;  // a seat line needs both a rating and a name
        int rating = 0;
        try {
            rating = std::stoi(rest->keyword);
        } catch (const std::exception&) {
            continue;  // an unreadable rating: skip this seat line
        }
        const std::string name = rest->value;
        if (field->keyword == whiteSeatKeyword) {
            roster.whiteName = name;
            roster.whiteRating = rating;
        } else if (field->keyword == blackSeatKeyword) {
            roster.blackName = name;
            roster.blackRating = rating;
        }
        // Any other keyword is unknown and ignored, mirroring the state decoder.
    }
    return roster;
}

std::string encodeAuthRejected(const AuthRejected& rejected) {
    return join(authFailTag, rejected.reason);
}

std::optional<AuthRejected> decodeAuthRejected(const std::string& payload) {
    return AuthRejected{payload};
}

}  // namespace

std::string encode(const WireMessage& message, int boardHeight) {
    return std::visit(
        [boardHeight](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, PlayerCommand>) {
                return join(commandTag, encodeCommand(value, boardHeight));
            } else if constexpr (std::is_same_v<T, RoleAssignment>) {
                return encodeRole(value);
            } else if constexpr (std::is_same_v<T, model::MoveEvent>) {
                return join(moveTag, encodeMoveEvent(value, boardHeight));
            } else if constexpr (std::is_same_v<T, model::CapturedPiece>) {
                return join(captureTag, encodeCapturedPiece(value));
            } else if constexpr (std::is_same_v<T, StateUpdate>) {
                return join(stateTag, value.frame);
            } else if constexpr (std::is_same_v<T, Login>) {
                return encodeLogin(value);
            } else if constexpr (std::is_same_v<T, PlayerRoster>) {
                return encodeRoster(value);
            } else if constexpr (std::is_same_v<T, AuthRejected>) {
                return encodeAuthRejected(value);
            } else if constexpr (std::is_same_v<T, SeekGame>) {
                return std::string(seekTag);
            } else if constexpr (std::is_same_v<T, CancelSeek>) {
                return std::string(cancelSeekTag);
            } else if constexpr (std::is_same_v<T, CreateRoom>) {
                return std::string(createRoomTag);
            } else if constexpr (std::is_same_v<T, JoinRoom>) {
                return join(joinRoomTag, value.roomId);
            } else if constexpr (std::is_same_v<T, EnteredRoom>) {
                return join(enteredRoomTag, value.roomId);
            } else if constexpr (std::is_same_v<T, NoMatch>) {
                return std::string(noMatchTag);
            } else if constexpr (std::is_same_v<T, RoomError>) {
                return join(roomErrorTag, value.reason);
            } else if constexpr (std::is_same_v<T, OpponentDisconnected>) {
                return join(oppGoneTag, std::to_string(value.secondsLeft));
            } else {  // OpponentReconnected
                return std::string(oppBackTag);
            }
        },
        message);
}

std::optional<WireMessage> decode(const std::string& text, int boardHeight) {
    const Split parts = split(text);

    if (parts.tag == commandTag) {
        if (const std::optional<PlayerCommand> command =
                decodeCommand(parts.payload, boardHeight)) {
            return WireMessage{*command};
        }
    } else if (parts.tag == roleTag) {
        if (const std::optional<RoleAssignment> role = decodeRole(parts.payload)) {
            return WireMessage{*role};
        }
    } else if (parts.tag == moveTag) {
        if (const std::optional<model::MoveEvent> event =
                decodeMoveEvent(parts.payload, boardHeight)) {
            return WireMessage{*event};
        }
    } else if (parts.tag == captureTag) {
        if (const std::optional<model::CapturedPiece> captured =
                decodeCapturedPiece(parts.payload)) {
            return WireMessage{*captured};
        }
    } else if (parts.tag == stateTag) {
        return WireMessage{StateUpdate{parts.payload}};
    } else if (parts.tag == loginTag) {
        if (const std::optional<Login> login = decodeLogin(parts.payload)) {
            return WireMessage{*login};
        }
    } else if (parts.tag == rosterTag) {
        if (const std::optional<PlayerRoster> roster = decodeRoster(parts.payload)) {
            return WireMessage{*roster};
        }
    } else if (parts.tag == authFailTag) {
        if (const std::optional<AuthRejected> rejected =
                decodeAuthRejected(parts.payload)) {
            return WireMessage{*rejected};
        }
    } else if (parts.tag == seekTag) {
        return WireMessage{SeekGame{}};
    } else if (parts.tag == cancelSeekTag) {
        return WireMessage{CancelSeek{}};
    } else if (parts.tag == createRoomTag) {
        return WireMessage{CreateRoom{}};
    } else if (parts.tag == joinRoomTag) {
        const std::string roomId = trim(parts.payload);
        if (!roomId.empty()) return WireMessage{JoinRoom{roomId}};
    } else if (parts.tag == enteredRoomTag) {
        const std::string roomId = trim(parts.payload);
        if (!roomId.empty()) return WireMessage{EnteredRoom{roomId}};
    } else if (parts.tag == noMatchTag) {
        return WireMessage{NoMatch{}};
    } else if (parts.tag == roomErrorTag) {
        return WireMessage{RoomError{parts.payload}};
    } else if (parts.tag == oppGoneTag) {
        try {
            return WireMessage{OpponentDisconnected{std::stoi(parts.payload)}};
        } catch (const std::exception&) {
            return std::nullopt;
        }
    } else if (parts.tag == oppBackTag) {
        return WireMessage{OpponentReconnected{}};
    }
    return std::nullopt;
}

}  // namespace kfc::io
