#include "shared/logic/io/include/event_codec.hpp"

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "shared/logic/io/include/move_notation.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/io/include/text.hpp"

namespace kfc::io {
namespace {

// A MoveEvent on the wire is its fields in order, space-separated:
//   <pieceToken> <from> <to> <timeMs> <isCapture> <isJump>
// e.g. "wN b1 c3 1500 0 0". The piece token, the squares, and the letters all
// come from the shared codecs, so no field spells out its own encoding here.
constexpr std::size_t moveFieldCount = 6;

// A captured piece is just its identity: "wP".
constexpr std::size_t capturedFieldCount = 1;

std::string encodeFlag(bool flag) { return flag ? "1" : "0"; }

std::optional<bool> decodeFlag(const std::string& token) {
    if (token == "1") return true;
    if (token == "0") return false;
    return std::nullopt;
}

}  // namespace

std::string encodeMoveEvent(const model::MoveEvent& event, int boardHeight) {
    return encodePieceToken(event.player, event.kind) + " " +
           squareName(event.from, boardHeight) + " " +
           squareName(event.to, boardHeight) + " " +
           std::to_string(event.timeMs) + " " + encodeFlag(event.isCapture) +
           " " + encodeFlag(event.isJump);
}

std::optional<model::MoveEvent> decodeMoveEvent(const std::string& text,
                                                int boardHeight) {
    const std::vector<std::string> fields = tokenize(text);
    if (fields.size() != moveFieldCount) return std::nullopt;

    const std::optional<PieceCode> piece = pieceFromToken(fields[0]);
    const std::optional<model::Position> from =
        squareFromName(fields[1], boardHeight);
    const std::optional<model::Position> to =
        squareFromName(fields[2], boardHeight);
    const std::optional<bool> isCapture = decodeFlag(fields[4]);
    const std::optional<bool> isJump = decodeFlag(fields[5]);
    if (!piece || !from || !to || !isCapture || !isJump) return std::nullopt;

    int timeMs = 0;
    try {
        timeMs = std::stoi(fields[3]);
    } catch (const std::exception&) {
        return std::nullopt;
    }

    return model::MoveEvent{timeMs,      piece->color, piece->kind, *from,
                            *to,         *isCapture,   *isJump};
}

std::string encodeCapturedPiece(const model::CapturedPiece& captured) {
    return encodePieceToken(captured.color, captured.kind);
}

std::optional<model::CapturedPiece> decodeCapturedPiece(const std::string& text) {
    const std::vector<std::string> fields = tokenize(text);
    if (fields.size() != capturedFieldCount) return std::nullopt;

    const std::optional<PieceCode> piece = pieceFromToken(fields[0]);
    if (!piece) return std::nullopt;

    return model::CapturedPiece{piece->kind, piece->color};
}

}  // namespace kfc::io
