#include "client/app/include/cli.hpp"

#include <cstddef>

#include "shared/logic/io/include/text.hpp"

namespace kfc::app {
namespace {

// Fallbacks when nothing was entered, so a player is never nameless and a login
// is never blank.
const char* const defaultUsername = "Player";
const char* const defaultPassword = "password";

// One trimmed line off the shell, or the fallback if it was blank.
std::string promptLine(std::istream& in, std::ostream& out, const char* label,
                       const char* fallback) {
    out << label << std::flush;
    std::string line;
    std::getline(in, line);
    const std::string trimmed = kfc::io::trim(line);
    return trimmed.empty() ? fallback : trimmed;
}

}  // namespace

ClientOptions parseClientOptions(const std::vector<std::string>& args) {
    ClientOptions options;
    if (!args.empty() && args[0] == "--script") {
        options.script = true;
        return options;
    }
    // Without --connect the game runs against a same-process loopback; with it,
    // against a real server. A "--"-prefixed following token is a flag, not the
    // address, so --connect alone keeps the default address.
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--connect") {
            options.connect = "";
            if (i + 1 < args.size() && args[i + 1].rfind("--", 0) != 0) {
                options.connect = args[++i];
            }
        } else {
            options.assetsRoot = args[i];
        }
    }
    return options;
}

Credentials promptCredentials(std::istream& in, std::ostream& out) {
    Credentials credentials;
    credentials.username =
        promptLine(in, out, "Enter your username: ", defaultUsername);
    credentials.password =
        promptLine(in, out, "Enter your password: ", defaultPassword);
    return credentials;
}

}  // namespace kfc::app
