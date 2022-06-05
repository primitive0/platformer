#pragma once

#ifndef NDEBUG
inline constexpr bool isDebugBuild = true;
#else
inline constexpr bool isDebugBuild = false;
#endif

#include <cstring>
#include <iostream>

extern bool debugEnabled;

inline void setupDebug() noexcept {
    const char* envDebug = std::getenv("DEBUG");
    debugEnabled = isDebugBuild || (envDebug != nullptr && strcmp(envDebug, "true") == 0);

    if (debugEnabled) {
        std::cout << "Debug enabled" << std::endl;
    }
}
