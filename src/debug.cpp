#include <iostream>
#include <cstring>

#include "debug.h"

bool debugEnabled = false;

void setupDebug() noexcept {
    const char* envDebug = std::getenv("DEBUG");
    debugEnabled = isDebugBuild || (envDebug != nullptr && strcmp(envDebug, "true") == 0);

    if (debugEnabled) {
        std::cout << "Debug enabled" << std::endl;
    }
}
