#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../debug.h"
#include "../../glfw.h"
#include "../../vk_validation_layers.h"
#include "../../window.h"

inline std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    return extensions;
}

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
