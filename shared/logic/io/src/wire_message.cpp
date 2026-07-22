#include "shared/logic/io/include/wire_message.hpp"

#include <string>

#include "shared/logic/io/include/event_codec.hpp"
#include "shared/logic/io/include/piece_codec.hpp"

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

// The role payload for a spectator, distinct from either colour letter.
constexpr char spectatorMark = '-';

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
            } else {  // StateUpdate
                return join(stateTag, value.frame);
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
    }
    return std::nullopt;
}

}  // namespace kfc::io
