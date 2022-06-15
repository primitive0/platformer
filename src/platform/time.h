#pragma once

//#if defined(__linux__) || defined(__LINUX__)
//#include <cstdint>
//
//#include <sys/time.h> // linux header
//
//uint64_t unixUsecs() {
//    timeval tv{};
//    gettimeofday(&tv, nullptr);
//    return static_cast<uint64_t>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
//}
//#elif defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN32__) || defined(_MSC_VER)
//#include <cstdint>
//
//#include "../platform/win_api.h"
//
//uint64_t unixUsecs() {
//    UINT64 sysTime = 0;
//    GetSystemTimeAsFileTime((FILETIME*)(void*)&sysTime);
//
//    sysTime /= 10000;
//    sysTime -= 11644473600000LL;
//    return static_cast<uint64_t>(sysTime);
//}
//#else
//#include <cstdint>
//#include <chrono>
//
//uint64_t unixUsecs() {
//    auto clock = std::chrono::system_clock::now();
//    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock.time_since_epoch()).count();
//    return static_cast<uint64_t>(ms);
//}
//#endif

#include <cstdint>
#include <chrono>

// Returns microseconds since unix epoch
uint64_t unixUsecs() {
    auto clock = std::chrono::system_clock::now();
    auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(clock.time_since_epoch()).count();
    return static_cast<uint64_t>(usecs);
}