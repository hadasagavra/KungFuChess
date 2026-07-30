#include "third_party/doctest/doctest.h"

#include <sstream>

#include "client/app/include/cli.hpp"

using kfc::app::ClientOptions;
using kfc::app::Credentials;
using kfc::app::parseClientOptions;
using kfc::app::promptCredentials;

TEST_CASE("no arguments means a local graphical game") {
    const ClientOptions options = parseClientOptions({});
    CHECK_FALSE(options.script);
    CHECK_FALSE(options.connect);
    CHECK(options.assetsRoot == "client/assets");
}

TEST_CASE("--script chooses the text harness") {
    CHECK(parseClientOptions({"--script"}).script);
}

TEST_CASE("--connect with an address is a networked game") {
    const ClientOptions options = parseClientOptions({"--connect", "1.2.3.4:9000"});
    REQUIRE(options.connect);
    CHECK(*options.connect == "1.2.3.4:9000");
}

TEST_CASE("--connect alone keeps the default address") {
    const ClientOptions options = parseClientOptions({"--connect"});
    REQUIRE(options.connect);  // networked...
    CHECK(options.connect->empty());  // ...at the default address
}

TEST_CASE("a bare argument is the assets root") {
    CHECK(parseClientOptions({"some/assets"}).assetsRoot == "some/assets");
}

TEST_CASE("blank credentials fall back to defaults") {
    std::istringstream in{"\n\n"};  // two blank lines
    std::ostringstream out;
    const Credentials credentials = promptCredentials(in, out);
    CHECK(credentials.username == "Player");
    CHECK(credentials.password == "password");
}

TEST_CASE("credentials are trimmed") {
    std::istringstream in{"  alice \n  s3cret \n"};
    std::ostringstream out;
    const Credentials credentials = promptCredentials(in, out);
    CHECK(credentials.username == "alice");
    CHECK(credentials.password == "s3cret");
}
