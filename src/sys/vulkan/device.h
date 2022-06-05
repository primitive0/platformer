#pragma once

#include "physical_device.h"

inline constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;

inline void initializeQueueCreateInfo(VkDeviceQueueCreateInfo& info, uint32_t familyIndex) {
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = familyIndex;
    info.queueCount = 1;
    info.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
}

class Device {
private:
    VkDevice handle;

public:
    Device() noexcept : handle(VK_NULL_HANDLE) {}

    explicit Device(VkDevice device) noexcept : handle(device) {}

    static Device create(const PhysicalDevice& physicalDevice) {
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

        VkDevice device = VK_NULL_HANDLE;
        if (vkCreateDevice(physicalDevice.handle, &createInfo, nullptr, &device) == VK_SUCCESS) {
            return Device(device);
        } else {
            throw std::runtime_error("failed to create logical device");
        }
    }

    VkDevice getHandle() const noexcept {
        return handle;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) const {
        if (code.size() % 4 != 0) {
            throw std::runtime_error("failed to create shader module: code size does not divide by 4");
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(handle, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
        return shaderModule;
    }

    VkQueue getDeviceQueue(uint32_t queueFamilyIndex) const noexcept {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(handle, queueFamilyIndex, 0, &queue);
        return queue;
    }

    VkCommandPool createCommandPool(uint32_t familyIndex) const noexcept {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = familyIndex;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        vkCreateCommandPool(handle, &poolInfo, nullptr, &commandPool);
        return commandPool;
    }

    void destroyCommandPool(VkCommandPool commandPool) const noexcept {
        vkDestroyCommandPool(handle, commandPool, nullptr);
    }

    VkCommandBuffer allocateCommandBuffer(VkCommandPool commandPool) const noexcept {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(handle, &allocInfo, &commandBuffer);
        return commandBuffer;
    }

    VkFence createFence() const noexcept {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(handle, &fenceInfo, nullptr, &fence);
        return fence;
    }

    void destroyFence(VkFence fence) const noexcept {
        vkDestroyFence(handle, fence, nullptr);
    }

    VkSemaphore createSemaphore() const noexcept {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(handle, &semaphoreInfo, nullptr, &semaphore);
        return semaphore;
    }

    void destroySemaphore(VkSemaphore semaphore) const noexcept {
        vkDestroySemaphore(handle, semaphore, nullptr);
    }

    VkBuffer createBuffer(const VkBufferCreateInfo& info) {
        VkBuffer buffer = VK_NULL_HANDLE;
        vkCreateBuffer(handle, &info, nullptr, &buffer);
        return buffer;
    }

    void destroyBuffer(VkBuffer buffer) const noexcept {
        vkDestroyBuffer(handle, buffer, nullptr);
    }

    void freeMemory(VkDeviceMemory memory) const noexcept {
        vkFreeMemory(handle, memory, nullptr);
    }

    void waitIdle() const noexcept {
        vkDeviceWaitIdle(handle);
    }

    void destroy() const noexcept {
        vkDestroyDevice(handle, nullptr);
    }
};
