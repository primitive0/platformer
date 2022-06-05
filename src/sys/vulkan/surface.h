#pragma once

#include <stdexcept>

#include "../../window.h"

inline VkSurfaceKHR createVulkanSurface(VkInstance instance, const Window& window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window.getHandle(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vulkan surface");
    }
    return surface;
}
