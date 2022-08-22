#pragma once

#include <vector>

#include "app_info.h"
#include "debug.h"
#include "vk_validation_layers.h"
#include "util/slice.h"

inline VkInstance createVulkanInstance1(immslice<const char*> requiredExtensions, immslice<const char*> validationLayers) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = APP_INFO.name;
    appInfo.applicationVersion = APP_INFO.version;
    appInfo.pEngineName = APP_INFO.engineName;
    appInfo.engineVersion = APP_INFO.engineVersion;
    appInfo.apiVersion = VK_API_VERSION_1_0; // TODO: maybe add constant later

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.len());
    createInfo.ppEnabledExtensionNames = !requiredExtensions.isEmpty() ? requiredExtensions.data() : nullptr;
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.len());
    createInfo.ppEnabledLayerNames = !validationLayers.isEmpty() ? validationLayers.data() : nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_SUCCESS) {
        return instance;
    } else {
        throw std::runtime_error("failed to create vulkan instance");
    }
}

inline std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) {
        return {};
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    return devices;
}

inline std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties1(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    if (count == 0) {
        return {};
    }
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilies.data());
    return queueFamilies;
}

inline std::vector<VkSurfaceFormatKHR> getPhysicalDeviceSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    if (count == 0) {
        return {};
    }
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());
    return formats;
}

inline std::vector<VkPresentModeKHR> getPhysicalDeviceSurfacePresentModes(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    if (count == 0) {
        return {};
    }
    std::vector<VkPresentModeKHR> presentModes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, presentModes.data());
    return presentModes;
}

struct QueueFamilyIndices1 {
    uint32_t graphics = NUM_MAX<uint32_t>;
    uint32_t present = NUM_MAX<uint32_t>;

    bool isValid() const noexcept {
        return graphics != NUM_MAX<uint32_t> && present != NUM_MAX<uint32_t>;
    }
};

struct SwapChainSupportDetails1 {
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};

    bool isValid() const noexcept {
        return !formats.empty() && !presentModes.empty();
    }
};

struct PhysicalDeviceInfo {
    QueueFamilyIndices1 indices;
    SwapChainSupportDetails1 swapChainSupportDetails;
};

struct PhysicalDeviceEntry {
    static inline constexpr uint16_t SCORE_DISCRETE_GPU = 1000;

    VkPhysicalDevice handle = VK_NULL_HANDLE;
    PhysicalDeviceInfo info{};

    uint16_t score = 0;

    PhysicalDeviceEntry() = default;

    PhysicalDeviceEntry(VkPhysicalDevice handle, PhysicalDeviceInfo&& info, uint16_t score) {
        this->handle = handle;
        this->info = std::move(info);
        this->score = score;
    }
};

struct RenderDevice final { // TODO: maybe rename
private:
    static inline constexpr std::array<const char*, 2> REQUIRED_DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MULTI_DRAW_EXTENSION_NAME};

    static inline constexpr float DEVICE_QUEUE_DEFAULT_PRIORITY = 1.0f;

    Window window;

    VkInstance instance = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    explicit RenderDevice(Window window) : window(window) {}

    void init() {
        immslice<const char*> validationLayers;
        if (debugEnabled) {
            validationLayers = VALIDATION_LAYERS;
        }
        auto requiredExtensions = window.getRequiredVulkanExtensions();
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        instance = createVulkanInstance1(requiredExtensions, validationLayers);
        surface = window.createVkSurface(instance);
        auto physicalDeviceInfo = findPhysicalDevice1();
        createLogicalDevice(physicalDeviceInfo);
        loadDeviceQueues(physicalDeviceInfo);
    }

    QueueFamilyIndices1 findQueueFamilies1(VkPhysicalDevice device) {
        uint32_t graphicsFamily = NUM_MAX<uint32_t>;
        uint32_t presentFamily = NUM_MAX<uint32_t>;

        std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceQueueFamilyProperties1(device);
        for (size_t i = 0; i < queueFamilies.size(); i++) {
            const auto& queueFamily = queueFamilies[i];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                presentFamily = i;
            }
        }

        return {graphicsFamily, presentFamily};
    }

    SwapChainSupportDetails1 getDeviceSwapChainSupportDetails(VkPhysicalDevice device) {
        SwapChainSupportDetails1 details;
        details.formats = getPhysicalDeviceSurfaceFormats(device, surface);
        details.presentModes = getPhysicalDeviceSurfacePresentModes(device, surface);
        return details;
    }

    static bool checkDeviceExtensionSupport1(immslice<VkExtensionProperties> availableExtensions) {
        for (const auto requiredExtension : REQUIRED_DEVICE_EXTENSIONS) {
            bool extensionFound = false;
            for (const auto extension : availableExtensions) {
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

    PhysicalDeviceInfo findPhysicalDevice1() {
        auto devices = enumeratePhysicalDevices(instance);

        std::vector<PhysicalDeviceEntry> deviceTop{};
        deviceTop.reserve(devices.size());

        for (const auto device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            std::cout << "found device: " << deviceProperties.deviceName << std::endl;

            if (!deviceFeatures.geometryShader) {
                continue;
            }

            auto availableExtensions = enumerateDeviceExtensionProperties(device);
            for (const auto& extension : availableExtensions) {
                std::cout << "device has this extension: " << extension.extensionName << std::endl;
            }
            if (!checkDeviceExtensionSupport1(availableExtensions)) {
                continue;
            }

            auto queueFamilyIndices = findQueueFamilies1(device);
            if (!queueFamilyIndices.isValid()) {
                continue;
            }

            auto swapChainSupportDetails = getDeviceSwapChainSupportDetails(device);
            if (!swapChainSupportDetails.isValid()) {
                continue;
            }

            uint32_t score = 0;
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += PhysicalDeviceEntry::SCORE_DISCRETE_GPU;
            }

            PhysicalDeviceInfo physicalDeviceInfo;
            physicalDeviceInfo.indices = queueFamilyIndices;
            physicalDeviceInfo.swapChainSupportDetails = std::move(swapChainSupportDetails);

            deviceTop.emplace_back(device, std::move(physicalDeviceInfo), score);
        }

        PhysicalDeviceEntry selected;
        for (auto& entry : deviceTop) {
            if (selected.score < entry.score) {
                selected = std::move(entry);
            }
        }
        if (selected.handle == VK_NULL_HANDLE) {
            throw std::runtime_error("suitable physical device not found");
        }
        physicalDevice = selected.handle;
        return std::move(selected.info);
    }

    void createLogicalDevice(const PhysicalDeviceInfo& physicalDeviceInfo) {
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.wideLines = true; // TODO: check for availability

        VkPhysicalDeviceMultiDrawFeaturesEXT multiDrawFeatures{};
        multiDrawFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT;
        multiDrawFeatures.multiDraw = VK_TRUE;
        multiDrawFeatures.pNext = nullptr;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.features = deviceFeatures;
        deviceFeatures2.pNext = &multiDrawFeatures;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        VkDeviceQueueCreateInfo queueCreateInfos[2]{};
        if (physicalDeviceInfo.indices.graphics == physicalDeviceInfo.indices.present) {
            createInfo.queueCreateInfoCount = 1;

            queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[0].queueFamilyIndex = physicalDeviceInfo.indices.graphics;
            queueCreateInfos[0].queueCount = 1;
            queueCreateInfos[0].pQueuePriorities = &DEVICE_QUEUE_DEFAULT_PRIORITY;
        } else {
            createInfo.queueCreateInfoCount = 2;

            queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[0].queueFamilyIndex = physicalDeviceInfo.indices.graphics;
            queueCreateInfos[0].queueCount = 1;
            queueCreateInfos[0].pQueuePriorities = &DEVICE_QUEUE_DEFAULT_PRIORITY;

            queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[1].queueFamilyIndex = physicalDeviceInfo.indices.present;
            queueCreateInfos[1].queueCount = 1;
            queueCreateInfos[1].pQueuePriorities = &DEVICE_QUEUE_DEFAULT_PRIORITY;
        }
        createInfo.pQueueCreateInfos = queueCreateInfos;

        createInfo.pEnabledFeatures = nullptr;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        createInfo.pNext = &deviceFeatures2;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device");
        }
    }

    void loadDeviceQueues(const PhysicalDeviceInfo& info) {
        if (info.indices.graphics == info.indices.present) {
            vkGetDeviceQueue(logicalDevice, info.indices.graphics, 0, &graphicsQueue);
            presentQueue = graphicsQueue;
        } else {
            vkGetDeviceQueue(logicalDevice, info.indices.graphics, 0, &graphicsQueue);
            vkGetDeviceQueue(logicalDevice, info.indices.present, 0, &presentQueue);
        }
    }

public:
    static RenderDevice create(Window window) {
        RenderDevice device(window);
        device.init();
        return device;
    }

    void destroy() const noexcept {
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};
