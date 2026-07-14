    #include "texttests/include/script_parser.hpp"

    #include <exception>
    #include <vector>

    #include "io/include/text.hpp"

    namespace kfc::texttests {
    namespace {

    constexpr char click[] = "click";
    constexpr char wait[] = "wait";
    constexpr char print[] = "print";
    constexpr char board[] = "board";
    constexpr char jump[] = "jump";

    }  // namespace

    std::optional<Command> parseCommand(const std::string& line) {
        std::vector<std::string> tokens = io::tokenize(line);
        if (tokens.empty()) return std::nullopt;

        try {
            if (tokens[0] == click && tokens.size() == 3)
                return ClickCommand{std::stoi(tokens[1]), std::stoi(tokens[2])};
            if (tokens[0] == jump && tokens.size() == 3)
                return JumpCommand{std::stoi(tokens[1]), std::stoi(tokens[2])};
            if (tokens[0] == wait && tokens.size() == 2)
                return WaitCommand{std::stoi(tokens[1])};
            if (tokens[0] == print && tokens.size() == 2 && tokens[1] == board)
                return PrintBoardCommand{};
        } catch (const std::exception&) {
            return std::nullopt;
        }

        return std::nullopt;
    }

    }