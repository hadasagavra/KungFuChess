#include "shared/logic/io/include/state_codec.hpp"

#include <exception>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "shared/logic/io/include/board_parser.hpp"
#include "shared/logic/io/include/board_printer.hpp"
#include "shared/logic/io/include/move_notation.hpp"
#include "shared/logic/io/include/text.hpp"

namespace kfc::io {
namespace {

// Each non-board line is named by its first token; a board row starts with a
// cell token ("wR", ".") and so never collides with these.
constexpr const char* overKeyword = "over";
constexpr const char* motionKeyword = "motion";
constexpr const char* cooldownKeyword = "cooldown";

// A motion line is "motion <from> <to> <progress>"; a cooldown line is
// "cooldown <cell> <progress>"; an over line is "over <flag>".
constexpr std::size_t motionFieldCount = 4;
constexpr std::size_t cooldownFieldCount = 3;
constexpr std::size_t overFieldCount = 2;

// Progress is a fraction in [0, 1]; three decimals is finer than a display can
// show and round-trips closely enough for a frame that is replaced next tick.
constexpr int progressDecimals = 3;

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

}  // namespace

std::string encodeState(const engine::GameSnapshot& snapshot) {
    std::ostringstream out;
    out << overKeyword << ' ' << encodeFlag(snapshot.isOver()) << '\n';
    printBoard(snapshot, out);
    encodeMotions(snapshot.motions(), snapshot.height(), out);
    encodeCooldowns(snapshot.cooldowns(), snapshot.height(), out);
    return out.str();
}

std::optional<DecodedState> decodeState(const std::string& text,
                                        model::Board& board) {
    // First pass: sort the lines. The board rows are collected as raw text for
    // buildBoard; the motion/cooldown lines are held as token lists to convert
    // once the board's height (which relates rows to ranks) is known.
    std::vector<std::string> boardRows;
    std::vector<std::vector<std::string>> motionLines;
    std::vector<std::vector<std::string>> cooldownLines;
    std::optional<bool> isOver;

    std::istringstream in{text};
    std::string line;
    while (std::getline(in, line)) {
        const std::vector<std::string> fields = tokenize(line);
        if (fields.empty()) continue;

        if (fields[0] == overKeyword) {
            if (fields.size() != overFieldCount) return std::nullopt;
            isOver = decodeFlag(fields[1]);
            if (!isOver) return std::nullopt;
        } else if (fields[0] == motionKeyword) {
            motionLines.push_back(fields);
        } else if (fields[0] == cooldownKeyword) {
            cooldownLines.push_back(fields);
        } else {
            boardRows.push_back(line);
        }
    }

    // The over flag and a board are both mandatory: the encoder always writes
    // them, so their absence means the text is not a frame.
    if (!isOver) return std::nullopt;
    std::optional<model::Board> rebuilt;
    try {
        rebuilt = buildBoard(boardRows);
    } catch (const ParseError&) {
        return std::nullopt;
    }
    const int boardHeight = rebuilt->height();

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

    // Commit only now that every part parsed, so a bad frame leaves the caller's
    // board untouched rather than half-updated.
    board = std::move(*rebuilt);
    return state;
}

}  // namespace kfc::io
