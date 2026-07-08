#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "BoardParser.h"
#include "Game.h"

namespace {

kfc::logic::Position pixelToCell(int x, int y) {
    return kfc::logic::Position{y / 100, x / 100};
}

std::vector<std::string> splitWords(const std::string& line) {
    std::vector<std::string> words;
    std::istringstream iss(line);
    std::string word;
    while (iss >> word) words.push_back(word);
    return words;
}

}  // namespace

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    kfc::logic::BoardParser parser;
    std::vector<std::string> commands;

    try {
        kfc::logic::Board board = parser.parse(std::cin, commands);
        kfc::logic::Game game(std::move(board));

        for (const auto& command : commands) {
            std::vector<std::string> words = splitWords(command);
            if (words.empty()) continue;

            if (words[0] == "click" && words.size() == 3) {
                int x = std::stoi(words[1]);
                int y = std::stoi(words[2]);
                game.handleClickCell(pixelToCell(x, y));
            } else if (words[0] == "wait" && words.size() == 2) {
                game.advanceClock(std::stoll(words[1]));
            } else if (command == "print board") {
                game.board().print(std::cout);
            }
        }
    } catch (const kfc::logic::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
    }

    return 0;
}