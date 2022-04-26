#pragma once

#include <limits>
#include <cstring>

#include "helper/vulkan.h"
#include "util/numbers.h"

inline constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;

inline constexpr std::array<const char*, 1> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct VulkanApplicationInfo {
    std::string applicationName;
    uint32_t applicationVersion;
    std::string engineName;
    uint32_t engineVersion;
};

VkInstance createVulkanInstance(const VulkanApplicationInfo& info) {
    VkInstance instance = VK_NULL_HANDLE;

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

    auto glfwExtensions = getGlfwRequiredVulkanExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
    createInfo.ppEnabledExtensionNames = glfwExtensions.data();

    if (debugEnabled) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    std::vector<const char*>().swap(glfwExtensions);

    if (result == VK_SUCCESS) {
        return instance;
    } else {
        throw std::runtime_error("failed to create vulkan instance");
    }
}

VkSurfaceKHR createVulkanSurface(VkInstance instance, const Window& window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window.getHandle(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vulkan surface");
    }
    return surface;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    auto availableExtensions = enumerateDeviceExtensionProperties(device);
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

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;

    QueueFamilyIndices(uint32_t graphicsFamily, uint32_t presentFamily) noexcept: graphicsFamily(graphicsFamily), presentFamily(presentFamily) {}
};

std::optional<QueueFamilyIndices> findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
            QueueFamilyIndices indices(graphicsFamily.value(), presentFamily.value());
            return indices;
        }

        i++;
    }

    return std::nullopt;
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    SwapChainSupportDetails() noexcept: capabilities({}), formats({}), presentModes({}) {};

    bool isUsable() const noexcept {
        return !formats.empty() && !presentModes.empty();
    }
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

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

struct VulkanPhysicalDevice {
    VkPhysicalDevice handle;
    QueueFamilyIndices familyIndices;
    SwapChainSupportDetails swapChainSupportDetails;

    VulkanPhysicalDevice() : handle(VK_NULL_HANDLE), familyIndices({NUM_MAX<uint32_t>, NUM_MAX<uint32_t>}),
                             swapChainSupportDetails({}) {} // TODO: create better constructor
};

VulkanPhysicalDevice findPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
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

            if (!(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader)) {
                continue;
            }

            if (!checkDeviceExtensionSupport(device)) {
                continue;
            }

            auto familyIndices = findQueueFamilies(device, surface);
            if (!familyIndices.has_value()) {
                continue;
            }

            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device, surface);
            if (!swapChainSupportDetails.isUsable()) {
                continue;
            }

            VulkanPhysicalDevice physicalDevice{};
            physicalDevice.handle = device;
            physicalDevice.familyIndices = familyIndices.value();
            physicalDevice.swapChainSupportDetails = swapChainSupportDetails;
            return physicalDevice;
        }
    }

    throw std::runtime_error("suitable physical device not found");
}

inline void initializeQueueCreateInfo(VkDeviceQueueCreateInfo& info, uint32_t familyIndex) {
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = familyIndex;
    info.queueCount = 1;
    info.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
}

VkDevice createVulkanLogicalDevice(const VulkanPhysicalDevice& physicalDevice) {
    VkDevice device = VK_NULL_HANDLE;

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkDeviceQueueCreateInfo queueCreateInfos[2]{};
    if (physicalDevice.familyIndices.graphicsFamily == physicalDevice.familyIndices.presentFamily) {
        createInfo.queueCreateInfoCount = 1;
        initializeQueueCreateInfo(queueCreateInfos[0], physicalDevice.familyIndices.graphicsFamily);
    } else {
        createInfo.queueCreateInfoCount = 2;
        initializeQueueCreateInfo(queueCreateInfos[0], physicalDevice.familyIndices.graphicsFamily);
        initializeQueueCreateInfo(queueCreateInfos[1], physicalDevice.familyIndices.presentFamily);
    }
    createInfo.pQueueCreateInfos = queueCreateInfos;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (vkCreateDevice(physicalDevice.handle, &createInfo, nullptr, &device) == VK_SUCCESS) {
        return device;
    } else {
        throw std::runtime_error("failed to create logical device");
    }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    throw std::runtime_error("failed to find suitable surface format");
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D windowExtent) {
    if (capabilities.currentExtent.width != NUM_MAX<uint32_t>) {
        return capabilities.currentExtent;
    } else {
        uint32_t width = std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        uint32_t height = std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return {width, height};
    }
}

struct SwapChainCreateDetails {
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
};

SwapChainCreateDetails lookupSwapChainCreateDetails(const VulkanPhysicalDevice& physicalDevice, const Window& window) {
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(physicalDevice.swapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice.swapChainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(physicalDevice.swapChainSupportDetails.capabilities, window.getWindowExtent());
    return {surfaceFormat, presentMode, extent};
}

VkSwapchainKHR createSwapChain(const VulkanPhysicalDevice& physicalDevice, VkDevice device, VkSurfaceKHR surface, SwapChainCreateDetails createDetails) {
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;

    auto& supportDetails = physicalDevice.swapChainSupportDetails;

    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
    if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
        imageCount = supportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = createDetails.surfaceFormat.format;
    createInfo.imageColorSpace = createDetails.surfaceFormat.colorSpace;
    createInfo.imageExtent = createDetails.extent;
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

    createInfo.preTransform = supportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = createDetails.presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
    }

    return swapChain;
}
