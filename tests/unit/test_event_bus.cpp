#include "third_party/doctest/doctest.h"

#include <string>
#include <vector>

#include "shared/bus/include/event_bus.hpp"

using kfc::bus::EventBus;

namespace {

// Two unrelated event types. They are deliberately not game events: the bus
// knows nothing about chess, and these tests prove it by never mentioning it.
struct Ping {
    int value;
};

struct Pong {
    std::string label;
};

}  // namespace

TEST_CASE("A subscriber receives the event it asked for") {
    EventBus bus;
    std::vector<int> received;
    bus.subscribe<Ping>([&received](const Ping& ping) {
        received.push_back(ping.value);
    });

    bus.publish(Ping{7});

    REQUIRE(received.size() == 1);
    CHECK(received[0] == 7);
}

TEST_CASE("An event is delivered only to subscribers of its own type") {
    EventBus bus;
    int pings = 0;
    int pongs = 0;
    bus.subscribe<Ping>([&pings](const Ping&) { ++pings; });
    bus.subscribe<Pong>([&pongs](const Pong&) { ++pongs; });

    bus.publish(Pong{"only this one"});

    CHECK(pings == 0);
    CHECK(pongs == 1);
}

TEST_CASE("Every subscriber to a type hears the event, in subscription order") {
    EventBus bus;
    std::vector<std::string> order;
    bus.subscribe<Ping>([&order](const Ping&) { order.push_back("first"); });
    bus.subscribe<Ping>([&order](const Ping&) { order.push_back("second"); });

    bus.publish(Ping{0});

    REQUIRE(order.size() == 2);
    CHECK(order[0] == "first");
    CHECK(order[1] == "second");
}

TEST_CASE("Publishing with nobody listening does nothing") {
    EventBus bus;

    // A publisher never checks whether anyone is there, so this must be safe on
    // a bus with no channel at all and on one with an unrelated channel.
    bus.publish(Ping{1});
    bus.subscribe<Pong>([](const Pong&) {});
    bus.publish(Ping{2});

    CHECK(true);  // reaching here without a crash is the assertion
}

TEST_CASE("One subscriber can listen to several event types") {
    EventBus bus;
    std::vector<std::string> heard;
    bus.subscribe<Ping>([&heard](const Ping&) { heard.push_back("ping"); });
    bus.subscribe<Pong>([&heard](const Pong&) { heard.push_back("pong"); });

    bus.publish(Ping{1});
    bus.publish(Pong{"x"});

    REQUIRE(heard.size() == 2);
    CHECK(heard[0] == "ping");
    CHECK(heard[1] == "pong");
}

TEST_CASE("The event's own data reaches the subscriber intact") {
    EventBus bus;
    std::string seen;
    bus.subscribe<Pong>([&seen](const Pong& pong) { seen = pong.label; });

    bus.publish(Pong{"carried through"});

    CHECK(seen == "carried through");
}
