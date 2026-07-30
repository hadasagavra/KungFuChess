#include "shared/logic/io/include/wire_message.hpp"

#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "shared/logic/io/include/event_codec.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/io/include/text.hpp"

namespace kfc::io {
namespace {

// ===========================================================================
// One message, one place. Each WireMessage alternative has a WireCodec<T>
// specialization that is the SINGLE source of truth for how that message sits on
// the wire: its tag, how its payload is written, and how it is read back. The
// generic encode()/decode() below drive everything off these specializations, so
// there is no parallel tag table and no per-message if/else chain to keep in
// sync -- adding a message is one specialization plus one entry in the variant,
// and forgetting the specialization is a compile error (WireCodec<T> undefined).
//
// A codec provides:
//   static constexpr std::string_view tag;
//   static std::string encodePayload(const T&, int boardHeight);   // no tag
//   static std::optional<T> decodePayload(std::string_view, int);  // no tag
// The board height is passed to every codec for a uniform signature; codecs that
// carry no board square simply ignore it.
// ===========================================================================
template <typename T>
struct WireCodec;  // no primary definition: every message must specialize it

// A tag joined to its payload. The payload may hold spaces and newlines (a state
// frame does); only the first space, the one this inserts, separates the two.
std::string join(std::string_view tag, const std::string& payload) {
    std::string message(tag);
    message += ' ';
    message += payload;
    return message;
}

// A message split at its first space into the tag and the payload after it.
struct Split {
    std::string_view tag;
    std::string_view payload;
};

Split split(std::string_view message) {
    const std::size_t space = message.find(' ');
    if (space == std::string_view::npos) return {message, {}};
    return {message.substr(0, space), message.substr(space + 1)};
}

// -- Payload-internal vocabulary -------------------------------------------------
// These name the structure INSIDE a payload (not the message tags), shared by the
// one message's encode and decode. The spectator role marker, and the line
// keywords used by the login and roster payloads.
constexpr char spectatorMark = '-';  // distinct from either colour letter
constexpr const char* userKeyword = "user";
constexpr const char* passKeyword = "pass";
constexpr const char* whiteSeatKeyword = "white";
constexpr const char* blackSeatKeyword = "black";

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

// -- The role payload: one colour letter, or the spectator mark ------------------
std::string encodeRolePayload(const RoleAssignment& role) {
    return std::string(1, role.color ? colorLetter(*role.color) : spectatorMark);
}

std::optional<RoleAssignment> decodeRolePayload(std::string_view payload) {
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

// -- The login payload: a "user" line and a "pass" line --------------------------
std::string encodeLoginPayload(const Login& login) {
    return std::string(userKeyword) + ' ' + login.username + '\n' + passKeyword +
           ' ' + login.password + '\n';
}

std::optional<Login> decodeLoginPayload(std::string_view payload) {
    std::optional<std::string> username;
    std::optional<std::string> password;
    std::istringstream in{std::string(payload)};
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

// -- The roster payload: a "<seat> <rating> <name>" line per named seat ----------
std::string rosterLine(const char* seatKeyword, int rating,
                       const std::string& name) {
    return std::string(seatKeyword) + ' ' + std::to_string(rating) + ' ' + name +
           '\n';
}

std::string encodeRosterPayload(const PlayerRoster& roster) {
    std::string payload;
    if (roster.whiteName) {
        payload += rosterLine(whiteSeatKeyword, roster.whiteRating.value_or(0),
                              *roster.whiteName);
    }
    if (roster.blackName) {
        payload += rosterLine(blackSeatKeyword, roster.blackRating.value_or(0),
                              *roster.blackName);
    }
    return payload;
}

std::optional<PlayerRoster> decodeRosterPayload(std::string_view payload) {
    PlayerRoster roster;
    std::istringstream in{std::string(payload)};
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
        if (field->keyword == whiteSeatKeyword) {
            roster.whiteName = rest->value;
            roster.whiteRating = rating;
        } else if (field->keyword == blackSeatKeyword) {
            roster.blackName = rest->value;
            roster.blackRating = rating;
        }
        // Any other keyword is unknown and ignored, mirroring the state decoder.
    }
    return roster;
}

// A payload that is a single non-blank id (a room id). Empty is not a message.
std::optional<std::string> nonEmptyId(std::string_view payload) {
    const std::string id = trim(std::string(payload));
    if (id.empty()) return std::nullopt;
    return id;
}

// ===========================================================================
// The codecs. Each is short and self-contained; the interesting per-message
// logic lives in the helpers above, reused verbatim.
// ===========================================================================

template <>
struct WireCodec<PlayerCommand> {
    static constexpr std::string_view tag = "CMD";
    static std::string encodePayload(const PlayerCommand& m, int h) {
        return encodeCommand(m, h);
    }
    static std::optional<PlayerCommand> decodePayload(std::string_view p, int h) {
        return decodeCommand(std::string(p), h);
    }
};

template <>
struct WireCodec<RoleAssignment> {
    static constexpr std::string_view tag = "ROLE";
    static std::string encodePayload(const RoleAssignment& m, int) {
        return encodeRolePayload(m);
    }
    static std::optional<RoleAssignment> decodePayload(std::string_view p, int) {
        return decodeRolePayload(p);
    }
};

template <>
struct WireCodec<model::MoveEvent> {
    static constexpr std::string_view tag = "MOVE";
    static std::string encodePayload(const model::MoveEvent& m, int h) {
        return encodeMoveEvent(m, h);
    }
    static std::optional<model::MoveEvent> decodePayload(std::string_view p,
                                                         int h) {
        return decodeMoveEvent(std::string(p), h);
    }
};

template <>
struct WireCodec<model::CapturedPiece> {
    static constexpr std::string_view tag = "CAPTURE";
    static std::string encodePayload(const model::CapturedPiece& m, int) {
        return encodeCapturedPiece(m);
    }
    static std::optional<model::CapturedPiece> decodePayload(std::string_view p,
                                                             int) {
        return decodeCapturedPiece(std::string(p));
    }
};

template <>
struct WireCodec<StateUpdate> {
    static constexpr std::string_view tag = "STATE";
    static std::string encodePayload(const StateUpdate& m, int) {
        return m.frame;
    }
    static std::optional<StateUpdate> decodePayload(std::string_view p, int) {
        return StateUpdate{std::string(p)};
    }
};

template <>
struct WireCodec<Login> {
    static constexpr std::string_view tag = "LOGIN";
    static std::string encodePayload(const Login& m, int) {
        return encodeLoginPayload(m);
    }
    static std::optional<Login> decodePayload(std::string_view p, int) {
        return decodeLoginPayload(p);
    }
};

template <>
struct WireCodec<PlayerRoster> {
    static constexpr std::string_view tag = "ROSTER";
    static std::string encodePayload(const PlayerRoster& m, int) {
        return encodeRosterPayload(m);
    }
    static std::optional<PlayerRoster> decodePayload(std::string_view p, int) {
        return decodeRosterPayload(p);
    }
};

template <>
struct WireCodec<AuthRejected> {
    static constexpr std::string_view tag = "AUTHFAIL";
    static std::string encodePayload(const AuthRejected& m, int) {
        return m.reason;
    }
    static std::optional<AuthRejected> decodePayload(std::string_view p, int) {
        return AuthRejected{std::string(p)};
    }
};

// -- Lobby: three tag-only requests (no payload) and their room/error replies ----
template <>
struct WireCodec<SeekGame> {
    static constexpr std::string_view tag = "SEEK";
    static std::string encodePayload(const SeekGame&, int) { return ""; }
    static std::optional<SeekGame> decodePayload(std::string_view, int) {
        return SeekGame{};
    }
};

template <>
struct WireCodec<CancelSeek> {
    static constexpr std::string_view tag = "CANCEL";
    static std::string encodePayload(const CancelSeek&, int) { return ""; }
    static std::optional<CancelSeek> decodePayload(std::string_view, int) {
        return CancelSeek{};
    }
};

template <>
struct WireCodec<CreateRoom> {
    static constexpr std::string_view tag = "CREATE";
    static std::string encodePayload(const CreateRoom&, int) { return ""; }
    static std::optional<CreateRoom> decodePayload(std::string_view, int) {
        return CreateRoom{};
    }
};

template <>
struct WireCodec<JoinRoom> {
    static constexpr std::string_view tag = "JOIN";
    static std::string encodePayload(const JoinRoom& m, int) { return m.roomId; }
    static std::optional<JoinRoom> decodePayload(std::string_view p, int) {
        if (const std::optional<std::string> id = nonEmptyId(p)) {
            return JoinRoom{*id};
        }
        return std::nullopt;
    }
};

template <>
struct WireCodec<EnteredRoom> {
    static constexpr std::string_view tag = "ROOM";
    static std::string encodePayload(const EnteredRoom& m, int) {
        return m.roomId;
    }
    static std::optional<EnteredRoom> decodePayload(std::string_view p, int) {
        if (const std::optional<std::string> id = nonEmptyId(p)) {
            return EnteredRoom{*id};
        }
        return std::nullopt;
    }
};

template <>
struct WireCodec<NoMatch> {
    static constexpr std::string_view tag = "NOMATCH";
    static std::string encodePayload(const NoMatch&, int) { return ""; }
    static std::optional<NoMatch> decodePayload(std::string_view, int) {
        return NoMatch{};
    }
};

template <>
struct WireCodec<RoomError> {
    static constexpr std::string_view tag = "ROOMERR";
    static std::string encodePayload(const RoomError& m, int) {
        return m.reason;
    }
    static std::optional<RoomError> decodePayload(std::string_view p, int) {
        return RoomError{std::string(p)};
    }
};

template <>
struct WireCodec<OpponentDisconnected> {
    static constexpr std::string_view tag = "OPPGONE";
    static std::string encodePayload(const OpponentDisconnected& m, int) {
        return std::to_string(m.secondsLeft);
    }
    static std::optional<OpponentDisconnected> decodePayload(std::string_view p,
                                                             int) {
        try {
            return OpponentDisconnected{std::stoi(std::string(p))};
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
};

template <>
struct WireCodec<OpponentReconnected> {
    static constexpr std::string_view tag = "OPPBACK";
    static std::string encodePayload(const OpponentReconnected&, int) {
        return "";
    }
    static std::optional<OpponentReconnected> decodePayload(std::string_view,
                                                            int) {
        return OpponentReconnected{};
    }
};

// If `tag` names message T, decode its payload into `out` and report the match.
// A matched-but-malformed payload still counts as a match (out stays empty), so
// the search stops rather than trying another codec.
template <typename T>
bool matchDecode(std::string_view tag, std::string_view payload, int height,
                 std::optional<WireMessage>& out) {
    if (tag != WireCodec<T>::tag) return false;
    if (std::optional<T> value = WireCodec<T>::decodePayload(payload, height)) {
        out = WireMessage{std::move(*value)};
    }
    return true;
}

// Try each alternative of the WireMessage variant in turn (a fold over its type
// list), stopping at the codec whose tag matches. The variant pointer is only a
// carrier for the type pack; it is never dereferenced.
template <typename... Ts>
std::optional<WireMessage> decodeVariant(const std::variant<Ts...>*,
                                         std::string_view tag,
                                         std::string_view payload, int height) {
    std::optional<WireMessage> out;
    (matchDecode<Ts>(tag, payload, height, out) || ...);
    return out;
}

}  // namespace

std::string encode(const WireMessage& message, int boardHeight) {
    return std::visit(
        [boardHeight](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            return join(WireCodec<T>::tag,
                        WireCodec<T>::encodePayload(value, boardHeight));
        },
        message);
}

std::optional<WireMessage> decode(const std::string& text, int boardHeight) {
    const Split parts = split(text);
    return decodeVariant(static_cast<const WireMessage*>(nullptr), parts.tag,
                         parts.payload, boardHeight);
}

}  // namespace kfc::io
