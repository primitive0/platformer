#pragma once

#include <cstdint>
#include "glfw.h"

struct ApplicationInfo {
    const char* name;
    uint32_t version;
    const char* engineName;
    uint32_t engineVersion;
};

inline constexpr ApplicationInfo APP_INFO = {
    "Platformer",
    VK_MAKE_API_VERSION(1, 0, 0, 0),
    "Platformer Engine",
    VK_MAKE_API_VERSION(1, 0, 0, 0)
};
