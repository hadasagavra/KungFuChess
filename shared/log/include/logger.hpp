#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace kfc::log {

// Neutral logging infrastructure: timestamped diagnostic lines to std::clog. Like
// shared/bus, it names no game or network concept -- a caller passes a component
// tag and a message -- so any layer, client or server, may use it while it
// depends on none. It is diagnostics, not a game rule, and carries no state.
//
// Format: "[HH:MM:SS.mmm] [component] message". Serialized by a single mutex so
// interleaved lines from different call sites stay whole.

inline std::mutex& logMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::string logTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto ms =
        duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t seconds = system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &seconds);  // MSVC: the reentrant form
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S") << '.' << std::setw(3)
        << std::setfill('0') << ms.count();
    return out.str();
}

// Write one line for `component` (e.g. "server", "client"). Thread-safe.
inline void write(const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> guard(logMutex());
    std::clog << '[' << logTimestamp() << "] [" << component << "] " << message
              << '\n';
}

}  // namespace kfc::log
