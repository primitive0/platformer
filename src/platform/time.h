#pragma once

#include <chrono>
#include <cstdint>

// Returns microseconds since unix epoch
inline uint64_t unixUsecs() {
    auto clock = std::chrono::system_clock::now();
    auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(clock.time_since_epoch()).count();
    return static_cast<uint64_t>(usecs);
}
