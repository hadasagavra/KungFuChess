#pragma once

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace kfc::app {

// Credentials a networked client enters on the shell before the window opens.
struct Credentials {
    std::string username;
    std::string password;
};

// What the client's command line asked for. The text harness (--script) is one
// mode; otherwise it is a graphical game -- networked when `connect` is present
// (its value is the address, possibly empty for defaults) or local loopback when
// it is absent. `assetsRoot` is where sprites are loaded from.
struct ClientOptions {
    bool script = false;
    std::optional<std::string> connect;
    std::string assetsRoot = "client/assets";
};

// Parse the client's arguments (everything after argv[0]). --script wins; other
// arguments are an optional "--connect [host:port]" and an optional assets root.
ClientOptions parseClientOptions(const std::vector<std::string>& args);

// Prompt for a username then a password on the shell. A blank entry falls back to
// a default, so a login is never nameless or empty (the server could not
// authenticate a blank one).
Credentials promptCredentials(std::istream& in, std::ostream& out);

}  // namespace kfc::app
