#pragma once

#include <stdexcept>
#include <string>

#include "../../debug.h"
#include "../../glfw.h"
#include "../../helper/vulkan/validation_layers.h"
#include "../../window.h"

struct VulkanApplicationInfo {
    std::string applicationName;
    uint32_t applicationVersion;
    std::string engineName;
    uint32_t engineVersion;
};

inline VkInstance createVulkanInstance(const VulkanApplicationInfo& info, const Window& window) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = info.applicationName.c_str();
    appInfo.applicationVersion = info.applicationVersion;
    appInfo.pEngineName = info.engineName.c_str();
    appInfo.engineVersion = info.engineVersion;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto requiredExtensions = window.getRequiredVulkanExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (debugEnabled) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    std::vector<const char*>().swap(requiredExtensions);

    if (result == VK_SUCCESS) {
        return instance;
    } else {
        throw std::runtime_error("failed to create vulkan instance");
    }
}
