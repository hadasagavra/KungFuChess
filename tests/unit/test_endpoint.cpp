#include "third_party/doctest/doctest.h"

#include "client/net/include/endpoint.hpp"

using kfc::net::Endpoint;
using kfc::net::parseEndpoint;

namespace {
constexpr char defaultHost[] = "127.0.0.1";
constexpr std::uint16_t defaultPort = 9000;
}  // namespace

TEST_CASE("a full host:port is read as given") {
    const std::optional<Endpoint> endpoint =
        parseEndpoint("192.168.1.5:1234", defaultHost, defaultPort);
    REQUIRE(endpoint);
    CHECK(endpoint->host == "192.168.1.5");
    CHECK(endpoint->port == 1234);
}

TEST_CASE("a bare host keeps the default port") {
    const std::optional<Endpoint> endpoint =
        parseEndpoint("example.com", defaultHost, defaultPort);
    REQUIRE(endpoint);
    CHECK(endpoint->host == "example.com");
    CHECK(endpoint->port == defaultPort);
}

TEST_CASE("a bare :port keeps the default host") {
    const std::optional<Endpoint> endpoint =
        parseEndpoint(":7777", defaultHost, defaultPort);
    REQUIRE(endpoint);
    CHECK(endpoint->host == defaultHost);
    CHECK(endpoint->port == 7777);
}

TEST_CASE("an empty string is both defaults") {
    const std::optional<Endpoint> endpoint =
        parseEndpoint("", defaultHost, defaultPort);
    REQUIRE(endpoint);
    CHECK(endpoint->host == defaultHost);
    CHECK(endpoint->port == defaultPort);
}

TEST_CASE("a non-numeric or out-of-range port is rejected") {
    CHECK_FALSE(parseEndpoint("host:abc", defaultHost, defaultPort));
    CHECK_FALSE(parseEndpoint("host:0", defaultHost, defaultPort));
    CHECK_FALSE(parseEndpoint("host:70000", defaultHost, defaultPort));
}
