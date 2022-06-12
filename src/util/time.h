#pragma once

#if defined(__linux__) || defined(__LINUX__)
#include <cstdint>

#include <sys/time.h> // linux header

uint64_t unixMs() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
#elif defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN32__) || defined(_MSC_VER)
#include <cstdint>

#include <Windows.h>

uint64_t unixMs() {
    UINT64 sysTime = 0;
    GetSystemTimeAsFileTime((FILETIME*)(void*)&sysTime);

    sysTime /= 10000;
    sysTime -= 11644473600000LL;
    return static_cast<uint64_t>(sysTime);
}
#else
#error Your platform is not supported
#endif
