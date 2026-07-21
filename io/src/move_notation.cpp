#include "io/include/move_notation.hpp"

#include "io/include/piece_codec.hpp"

namespace kfc::io {
namespace {

constexpr char firstFileLetter = 'a';
constexpr char captureMark = 'x';
constexpr const char* jumpMark = "J";

// A pawn is written without a letter of its own -- "e4", not "Pe4".
std::string pieceToken(model::Kind kind) {
    if (kind == model::Kind::Pawn) {
        return {};
    }
    return std::string(1, kindLetter(kind));
}

// The file a square sits on, as its letter: column 0 is the 'a' file.
char fileLetter(model::Position cell) {
    return static_cast<char>(firstFileLetter + cell.col);
}

// A capturing pawn is identified by the file it came from ("exd4"), since it has
// no piece letter to name it by.
std::string captureToken(const model::MoveEvent& move) {
    if (!move.isCapture) {
        return {};
    }
    std::string token;
    if (move.kind == model::Kind::Pawn) {
        token += fileLetter(move.from);
    }
    token += captureMark;
    return token;
}

}  // namespace

std::string squareName(model::Position cell, int boardHeight) {
    // Row 0 is the top of the board, which is the highest rank.
    const int rank = boardHeight - cell.row;
    return std::string(1, fileLetter(cell)) + std::to_string(rank);
}

std::string moveNotation(const model::MoveEvent& move, int boardHeight) {
    if (move.isJump) {
        return jumpMark + pieceToken(move.kind) +
               squareName(move.to, boardHeight);
    }
    return pieceToken(move.kind) + captureToken(move) +
           squareName(move.to, boardHeight);
}

}  // namespace kfc::io
