#pragma once

#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

#include "../../util/numbers.h"
#include "constants.h"

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;

    QueueFamilyIndices() noexcept : graphicsFamily(NUM_MAX<uint32_t>), presentFamily(NUM_MAX<uint32_t>) {}

    QueueFamilyIndices(uint32_t graphicsFamily, uint32_t presentFamily) noexcept : graphicsFamily(graphicsFamily), presentFamily(presentFamily) {}
};

struct SwapChainSupportDetails {
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};

    bool isUsable() const noexcept {
        return !formats.empty() && !presentModes.empty();
    }
};

struct PhysicalDevice {
    VkPhysicalDevice handle;
    QueueFamilyIndices familyIndices;
    SwapChainSupportDetails swapChainSupportDetails;

    PhysicalDevice() noexcept : handle(VK_NULL_HANDLE), familyIndices({}), swapChainSupportDetails({}) {}

    PhysicalDevice(VkPhysicalDevice handle, QueueFamilyIndices familyIndices, SwapChainSupportDetails&& swapChainSupportDetails) noexcept
        : handle(handle), familyIndices(familyIndices), swapChainSupportDetails(std::move(swapChainSupportDetails)) {}
};

inline std::vector<VkExtensionProperties> enumerateDeviceExtensionProperties(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensionProperties(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensionProperties.data());
    return extensionProperties;
}

inline bool checkDeviceExtensionSupport(const std::vector<VkExtensionProperties>& availableExtensions) {
    for (const auto& requiredExtension : DEVICE_EXTENSIONS) {
        bool extensionFound = false;
        for (const auto& extension : availableExtensions) {
            if (strcmp(requiredExtension, extension.extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound) {
            return false;
        }
    }
    return true;
}

inline std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilies.data());
    return queueFamilies;
}

inline std::optional<QueueFamilyIndices> findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceQueueFamilyProperties(device);

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            presentFamily = i;
        }

        if (graphicsFamily.has_value() && presentFamily.has_value()) {
            return QueueFamilyIndices(graphicsFamily.value(), presentFamily.value());
        }

        i++;
    }

    return std::nullopt;
}

inline SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details{};

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    details.formats = std::vector<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    details.presentModes = std::vector<VkPresentModeKHR>(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

    return details;
}

inline PhysicalDevice findPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount != 0) {
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            std::cout << "found device: " << deviceProperties.deviceName << std::endl;

            if (!(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader)) {
                continue;
            }

            auto availableExtensions = enumerateDeviceExtensionProperties(device);
            for (const auto& extension : availableExtensions) {
                std::cout << "device has this extension: " << extension.extensionName << std::endl;
            }

            if (!checkDeviceExtensionSupport(availableExtensions)) {
                continue;
            }

            auto findResult = findQueueFamilies(device, surface);
            if (!findResult.has_value()) {
                continue;
            }
            auto familyIndices = findResult.value();

            auto swapChainSupportDetails = querySwapChainSupport(device, surface);
            if (!swapChainSupportDetails.isUsable()) {
                continue;
            }

            return {device, familyIndices, std::move(swapChainSupportDetails)};
        }
    }

    throw std::runtime_error("suitable physical device not found");
}
