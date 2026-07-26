#include "shared/logic/io/include/state_codec.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "shared/logic/io/include/move_notation.hpp"
#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/io/include/text.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::io {
namespace {

// Each line is named by its first token.
constexpr const char* overKeyword = "over";
constexpr const char* sizeKeyword = "size";
constexpr const char* pieceKeyword = "piece";
constexpr const char* motionKeyword = "motion";
constexpr const char* cooldownKeyword = "cooldown";

// "over <flag>"; "size <w> <h>"; "piece <id> <token> <square> <state>";
// "motion <from> <to> <progress>"; "cooldown <cell> <progress>".
constexpr std::size_t overFieldCount = 2;
constexpr std::size_t sizeFieldCount = 3;
constexpr std::size_t pieceFieldCount = 5;
constexpr std::size_t motionFieldCount = 4;
constexpr std::size_t cooldownFieldCount = 3;

constexpr int progressDecimals = 3;

// The one place a piece's lifecycle state maps to a letter, so the frame the
// server sends rebuilds a piece with the same state it had -- which is what lets
// the view animate a replica exactly as it would a local board. Captured never
// appears on a board (a captured piece is off it), but is listed for totality.
struct StateLetter {
    model::State state;
    char letter;
};
constexpr std::array<StateLetter, 5> stateLetters{{
    {model::State::Idle, 'I'},
    {model::State::Moving, 'M'},
    {model::State::Airborne, 'A'},
    {model::State::Resting, 'R'},
    {model::State::Captured, 'C'},
}};

char stateLetter(model::State state) {
    for (const StateLetter& entry : stateLetters) {
        if (entry.state == state) return entry.letter;
    }
    return '?';
}

std::optional<model::State> stateFromLetter(char letter) {
    for (const StateLetter& entry : stateLetters) {
        if (entry.letter == letter) return entry.state;
    }
    return std::nullopt;
}

std::string encodeProgress(double progress) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(progressDecimals) << progress;
    return out.str();
}

std::optional<double> decodeProgress(const std::string& token) {
    try {
        return std::stod(token);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

void encodePieces(const engine::GameSnapshot& snapshot, std::ostream& out) {
    for (int row = 0; row < snapshot.height(); ++row) {
        for (int col = 0; col < snapshot.width(); ++col) {
            const model::Position cell{row, col};
            const std::optional<model::Piece> piece = snapshot.pieceAt(cell);
            if (!piece) continue;
            // id, then identity, then where it sits, then how it is behaving --
            // everything the view needs to place and animate it faithfully.
            out << pieceKeyword << ' ' << piece->getId() << ' '
                << encodePieceToken(piece->getColor(), piece->getKind()) << ' '
                << squareName(cell, snapshot.height()) << ' '
                << stateLetter(piece->getState()) << '\n';
        }
    }
}

void encodeMotions(const std::vector<realtime::MotionState>& motions,
                   int boardHeight, std::ostream& out) {
    for (const realtime::MotionState& motion : motions) {
        out << motionKeyword << ' ' << squareName(motion.from, boardHeight)
            << ' ' << squareName(motion.to, boardHeight) << ' '
            << encodeProgress(motion.progress) << '\n';
    }
}

void encodeCooldowns(const std::vector<realtime::CooldownState>& cooldowns,
                     int boardHeight, std::ostream& out) {
    for (const realtime::CooldownState& cooldown : cooldowns) {
        out << cooldownKeyword << ' ' << squareName(cooldown.cell, boardHeight)
            << ' ' << encodeProgress(cooldown.progress) << '\n';
    }
}

// A piece line held as raw tokens until the board size (which relates a square to
// a row) is known, then turned into a placed piece.
std::optional<std::shared_ptr<model::Piece>> decodePiece(
    const std::vector<std::string>& fields, int boardHeight) {
    if (fields.size() != pieceFieldCount) return std::nullopt;

    std::uint32_t id = 0;
    try {
        id = static_cast<std::uint32_t>(std::stoul(fields[1]));
    } catch (const std::exception&) {
        return std::nullopt;
    }
    const std::optional<PieceCode> code = pieceFromToken(fields[2]);
    const std::optional<model::Position> cell =
        squareFromName(fields[3], boardHeight);
    const std::optional<model::State> state =
        fields[4].size() == 1 ? stateFromLetter(fields[4][0]) : std::nullopt;
    if (!code || !cell || !state) return std::nullopt;

    return std::make_shared<model::Piece>(id, code->color, code->kind, *cell,
                                          *state);
}

std::optional<realtime::MotionState> decodeMotion(
    const std::vector<std::string>& fields, int boardHeight) {
    if (fields.size() != motionFieldCount) return std::nullopt;
    const std::optional<model::Position> from =
        squareFromName(fields[1], boardHeight);
    const std::optional<model::Position> to =
        squareFromName(fields[2], boardHeight);
    const std::optional<double> progress = decodeProgress(fields[3]);
    if (!from || !to || !progress) return std::nullopt;
    return realtime::MotionState{*from, *to, *progress};
}

std::optional<realtime::CooldownState> decodeCooldown(
    const std::vector<std::string>& fields, int boardHeight) {
    if (fields.size() != cooldownFieldCount) return std::nullopt;
    const std::optional<model::Position> cell =
        squareFromName(fields[1], boardHeight);
    const std::optional<double> progress = decodeProgress(fields[2]);
    if (!cell || !progress) return std::nullopt;
    return realtime::CooldownState{*cell, *progress};
}

// The board dimensions, parsed from a "size <w> <h>" line.
std::optional<std::pair<int, int>> decodeSize(
    const std::vector<std::string>& fields) {
    if (fields.size() != sizeFieldCount) return std::nullopt;
    try {
        const int width = std::stoi(fields[1]);
        const int height = std::stoi(fields[2]);
        if (width <= 0 || height <= 0) return std::nullopt;
        return std::make_pair(width, height);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

std::string encodeState(const engine::GameSnapshot& snapshot) {
    std::ostringstream out;
    out << overKeyword << ' ' << encodeFlag(snapshot.isOver()) << '\n';
    out << sizeKeyword << ' ' << snapshot.width() << ' ' << snapshot.height()
        << '\n';
    encodePieces(snapshot, out);
    encodeMotions(snapshot.motions(), snapshot.height(), out);
    encodeCooldowns(snapshot.cooldowns(), snapshot.height(), out);
    return out.str();
}

std::optional<DecodedState> decodeState(const std::string& text,
                                        model::Board& board) {
    // First pass: sort the lines. Pieces, motions and cooldowns are held as token
    // lists to convert once the size (which relates a square to a row) is known.
    std::optional<bool> isOver;
    std::optional<std::pair<int, int>> size;
    std::vector<std::vector<std::string>> pieceLines;
    std::vector<std::vector<std::string>> motionLines;
    std::vector<std::vector<std::string>> cooldownLines;

    std::istringstream in{text};
    std::string line;
    while (std::getline(in, line)) {
        const std::vector<std::string> fields = tokenize(line);
        if (fields.empty()) continue;

        if (fields[0] == overKeyword) {
            if (fields.size() != overFieldCount) return std::nullopt;
            isOver = decodeFlag(fields[1]);
            if (!isOver) return std::nullopt;
        } else if (fields[0] == sizeKeyword) {
            size = decodeSize(fields);
            if (!size) return std::nullopt;
        } else if (fields[0] == pieceKeyword) {
            pieceLines.push_back(fields);
        } else if (fields[0] == motionKeyword) {
            motionLines.push_back(fields);
        } else if (fields[0] == cooldownKeyword) {
            cooldownLines.push_back(fields);
        }
        // Any other line is unknown and ignored: a newer server may add fields a
        // client does not yet read, and the frame it does understand still stands.
    }

    // The over flag and the size are both mandatory: the encoder always writes
    // them, so their absence means the text is not a frame.
    if (!isOver || !size) return std::nullopt;
    const int boardHeight = size->second;

    // Build into a local board so a bad frame leaves the caller's untouched.
    model::Board rebuilt{size->first, size->second};
    try {
        for (const std::vector<std::string>& fields : pieceLines) {
            const std::optional<std::shared_ptr<model::Piece>> piece =
                decodePiece(fields, boardHeight);
            if (!piece) return std::nullopt;
            rebuilt.addPiece(*piece);
        }
    } catch (const std::exception&) {
        return std::nullopt;  // an out-of-bounds or occupied cell, etc.
    }

    DecodedState state{*isOver, {}, {}};
    for (const std::vector<std::string>& fields : motionLines) {
        const std::optional<realtime::MotionState> motion =
            decodeMotion(fields, boardHeight);
        if (!motion) return std::nullopt;
        state.motions.push_back(*motion);
    }
    for (const std::vector<std::string>& fields : cooldownLines) {
        const std::optional<realtime::CooldownState> cooldown =
            decodeCooldown(fields, boardHeight);
        if (!cooldown) return std::nullopt;
        state.cooldowns.push_back(*cooldown);
    }

    board = std::move(rebuilt);
    return state;
}

}  // namespace kfc::io
