#include "shared/logic/io/include/command_notation.hpp"

#include <cctype>
#include <cstddef>

#include "shared/logic/io/include/move_notation.hpp"
#include "shared/logic/io/include/piece_codec.hpp"

namespace kfc::io {
namespace {

// "WQ" + "e2" + "e5". The piece token is fixed at two characters; the two square
// names take whatever is left, split down the middle.
constexpr std::size_t pieceTokenLength = 2;

char toUpper(char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

char toLower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// A wire piece token differs from a board cell's only in the case of the colour
// letter, so the board form is produced first and then adjusted -- the letters
// themselves still come from piece_codec, never from a table kept here.
std::string toWireToken(model::Color color, model::Kind kind) {
    std::string token = encodePieceToken(color, kind);
    token[0] = toUpper(token[0]);
    return token;
}

std::optional<PieceCode> fromWireToken(const std::string& token) {
    std::string boardToken = token;
    boardToken[0] = toLower(boardToken[0]);
    return pieceFromToken(boardToken);
}

}  // namespace

std::string encodeCommand(const PlayerCommand& command, int boardHeight) {
    return toWireToken(command.player, command.kind) +
           squareName(command.from, boardHeight) +
           squareName(command.to, boardHeight);
}

std::optional<PlayerCommand> decodeCommand(const std::string& text,
                                           int boardHeight) {
    // Two square names of equal length must follow the piece token, so anything
    // else cannot be split into a source and a destination.
    if (text.size() <= pieceTokenLength) return std::nullopt;
    const std::size_t squaresLength = text.size() - pieceTokenLength;
    if (squaresLength % 2 != 0) return std::nullopt;
    const std::size_t squareLength = squaresLength / 2;

    const std::optional<PieceCode> piece =
        fromWireToken(text.substr(0, pieceTokenLength));
    if (!piece) return std::nullopt;

    const std::optional<model::Position> from =
        squareFromName(text.substr(pieceTokenLength, squareLength), boardHeight);
    const std::optional<model::Position> to =
        squareFromName(text.substr(pieceTokenLength + squareLength), boardHeight);
    if (!from || !to) return std::nullopt;

    return PlayerCommand{piece->color, piece->kind, *from, *to};
}

}  // namespace kfc::io
