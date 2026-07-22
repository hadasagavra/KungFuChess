#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kfc::net {

// A server address: where to reach it and on which port.
struct Endpoint {
    std::string host;
    std::uint16_t port;
};

// Parse "host:port", "host", ":port", or "" into an Endpoint, filling either
// half that was left out from the given defaults. nullopt when a port is present
// but is not a number in range -- a typed address is worth rejecting rather than
// silently dialling somewhere unintended. Pure and header-only, so it is unit
// tested without a socket.
inline std::optional<Endpoint> parseEndpoint(const std::string& text,
                                             const std::string& defaultHost,
                                             std::uint16_t defaultPort) {
    const std::size_t colon = text.find(':');
    const std::string host =
        (colon == std::string::npos) ? text : text.substr(0, colon);
    const std::string portText =
        (colon == std::string::npos) ? "" : text.substr(colon + 1);

    Endpoint endpoint{host.empty() ? defaultHost : host, defaultPort};
    if (!portText.empty()) {
        try {
            const int port = std::stoi(portText);
            if (port < 1 || port > 65535) return std::nullopt;
            endpoint.port = static_cast<std::uint16_t>(port);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return endpoint;
}

}  // namespace kfc::net
