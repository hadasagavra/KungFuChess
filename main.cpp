#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "Business Logic/BoardParser.h"

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    kfc::logic::BoardParser parser;
    std::vector<std::string> commands;

    try {
        kfc::logic::Board board = parser.parse(std::cin, commands);
        for (const auto& command : commands) {
            if (command == "print board") {
                board.print(std::cout);
            }
        }
    } catch (const kfc::logic::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
    }

    return 0;
}