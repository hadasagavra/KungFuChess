#pragma once

#include <optional>

namespace kfc::logic {

enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn };

constexpr char kWhiteColor = 'w';
constexpr char kBlackColor = 'b';
constexpr const char* kEmptyCellToken = ".";

bool isValidColor(char c);
std::optional<PieceType> charToPieceType(char c);
char pieceTypeToChar(PieceType type);

}  // namespace kfc::logic