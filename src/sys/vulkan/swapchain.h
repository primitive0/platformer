#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>

#include "../../util/numbers.h"
#include "../../window.h"
#include "device.h"

inline std::optional<VkSurfaceFormatKHR> chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) noexcept {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return std::nullopt;
}

inline VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) noexcept {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D windowExtent) noexcept {
    if (capabilities.currentExtent.width != NUM_MAX<uint32_t>) {
        return capabilities.currentExtent;
    } else {
        uint32_t width = std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        uint32_t height = std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return {width, height};
    }
}

struct SwapChainProperties {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
};

inline SwapChainProperties getSwapChainProperties(const PhysicalDevice& physicalDevice, const Window& window, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.handle, surface, &surfaceCapabilities);

    auto result = chooseSwapSurfaceFormat(physicalDevice.swapChainSupportDetails.formats);
    if (!result.has_value()) {
        throw std::runtime_error("failed to find suitable surface format");
    }
    auto surfaceFormat = result.value();
    auto presentMode = chooseSwapPresentMode(physicalDevice.swapChainSupportDetails.presentModes);
    auto extent = chooseSwapExtent(surfaceCapabilities, window.getWindowExtent());
    return {surfaceCapabilities, surfaceFormat, presentMode, extent};
}

inline VkSwapchainKHR createSwapChain(const PhysicalDevice& physicalDevice, VkDevice device, VkSurfaceKHR surface, SwapChainProperties properties) {
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;

    auto& supportDetails = physicalDevice.swapChainSupportDetails;

    uint32_t imageCount = properties.surfaceCapabilities.minImageCount + 1;
    if (properties.surfaceCapabilities.maxImageCount > 0 && imageCount > properties.surfaceCapabilities.maxImageCount) {
        imageCount = properties.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = properties.surfaceFormat.format;
    createInfo.imageColorSpace = properties.surfaceFormat.colorSpace;
    createInfo.imageExtent = properties.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto& familyIndices = physicalDevice.familyIndices;

    if (familyIndices.graphicsFamily == familyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    } else {
        uint32_t queueFamilyIndices[] = {familyIndices.graphicsFamily, familyIndices.presentFamily};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    createInfo.preTransform = properties.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = properties.presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
    }

    return swapChain;
}

inline std::vector<VkImageView> createImageViews(VkDevice device, const std::vector<VkImage>& swapChainImages, SwapChainProperties properties) {
    std::vector<VkImageView> swapChainImageViews;
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = properties.surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views");
        }
    }
    return swapChainImageViews;
}

inline std::vector<VkImage> getSwapChainImages(VkDevice device, VkSwapchainKHR swapChain) {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    return swapChainImages;
}

class SwapChain {
    VkSwapchainKHR swapChain;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    std::vector<VkImageView> imageViews;

    SwapChain(
        VkSwapchainKHR swapChain, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent, std::vector<VkImageView>&& imageViews
    ) noexcept
        : swapChain(swapChain), surfaceFormat(surfaceFormat), presentMode(presentMode), extent(extent), imageViews(std::move(imageViews)) {}

public:
    SwapChain() noexcept : swapChain(VK_NULL_HANDLE), surfaceFormat(), presentMode(), extent(), imageViews() {}

    static SwapChain create(PhysicalDevice& physicalDevice, Device device, const Window& window, VkSurfaceKHR surface) {
        auto swapChainProperties = getSwapChainProperties(physicalDevice, window, surface);
        auto swapChain = createSwapChain(physicalDevice, device.getHandle(), surface, swapChainProperties);
        auto swapChainImages = getSwapChainImages(device.getHandle(), swapChain);
        auto swapChainImageViews = createImageViews(device.getHandle(), swapChainImages, swapChainProperties);
        return {
            swapChain,
            swapChainProperties.surfaceFormat,
            swapChainProperties.presentMode,
            swapChainProperties.extent,
            std::move(swapChainImageViews),
        };
    }

    VkSwapchainKHR getSwapChainHandle() const noexcept {
        return swapChain;
    }

    VkSurfaceFormatKHR getSurfaceFormat() const noexcept {
        return surfaceFormat;
    }

    VkPresentModeKHR getPresentMode() const noexcept {
        return presentMode;
    }

    VkExtent2D getExtent() const noexcept {
        return extent;
    }

    const std::vector<VkImageView>& getImageViews() const noexcept {
        return imageViews;
    }

    void destroy(Device device) const noexcept {
        for (auto imageView : imageViews) {
            vkDestroyImageView(device.getHandle(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(device.getHandle(), swapChain, nullptr);
    }
};
