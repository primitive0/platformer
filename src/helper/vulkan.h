#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "../glfw.h"

inline constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
};

std::vector<const char*> getGlfwRequiredVulkanExtensions();

std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties();

std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device);

std::vector<VkExtensionProperties> enumerateDeviceExtensionProperties(VkPhysicalDevice device);

bool checkValidationLayerSupport();
