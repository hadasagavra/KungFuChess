#pragma once

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kfc::bus {

// A generic publish/subscribe bus. Publishers announce events by value and
// subscribers ask for exactly the event types they care about; neither side
// knows the other. The bus is deliberately ignorant of any concrete event
// type, so it depends on no game layer and any layer may depend on it.
//
// Adding a new kind of event costs nothing here and nothing to existing
// listeners: it is a new struct plus whoever cares to subscribe. That is the
// whole reason this replaced a fixed observer interface, where every new event
// meant editing a contract that all listeners shared.
//
// Three properties are deliberate and worth knowing before you use it:
//
//  - The event type is always written out at the call site --
//    subscribe<MoveEvent>(...) -- because a std::function parameter cannot
//    deduce it. Stating the channel where you join it reads better anyway.
//  - There is no unsubscribe. A handler usually captures the subscriber, so a
//    subscriber must outlive the bus it registered with.
//  - It is not thread-safe. Subscribing while a publish is in flight, or
//    touching the bus from two threads, is undefined. Today everything runs on
//    one thread; a networked server would need to revisit this.
class EventBus {
public:
    // Register interest in one event type. The handler is called on every
    // publish of exactly that type, in registration order.
    template <typename Event>
    void subscribe(std::function<void(const Event&)> handler) {
        channels_[std::type_index(typeid(Event))].push_back(
            [handler = std::move(handler)](const void* event) {
                handler(*static_cast<const Event*>(event));
            });
    }

    // Announce an event. Publishing to a type nobody listens for is a no-op, so
    // a publisher never has to check whether anyone is there. Const because
    // publishing changes nothing: a publisher can hold a const EventBus& and be
    // unable to subscribe through it.
    template <typename Event>
    void publish(const Event& event) const {
        auto found = channels_.find(std::type_index(typeid(Event)));
        if (found == channels_.end()) return;
        for (const ErasedHandler& handler : found->second) handler(&event);
    }

private:
    // Handlers are stored type-erased so the bus itself never names an event
    // type; each one restores its own type before calling the real handler.
    using ErasedHandler = std::function<void(const void*)>;

    std::unordered_map<std::type_index, std::vector<ErasedHandler>> channels_;
};

}  // namespace kfc::bus
