#include <iostream>
#include <cstring>

#include "debug.h"

bool debugEnabled = false;

void setupDebug() noexcept {
#ifndef NDEBUG
    debugEnabled = true;
#endif

    const char* envDebug = std::getenv("DEBUG");
    debugEnabled = debugEnabled || (envDebug != nullptr && strcmp(envDebug, "true") == 0);

    if (debugEnabled) {
        std::cout << "Debug enabled" << std::endl;
    }
}
