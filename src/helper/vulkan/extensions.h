#pragma once

#include <cstdint>
#include <vector>

#include "../../glfw.h"

inline std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    return extensions;
}
