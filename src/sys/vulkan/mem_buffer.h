#pragma once

#include "device.h"

inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

enum class MemBufferTransferDir {
    SOURCE,
    DESTINATION,
    NONE,
};

constexpr VkBufferUsageFlags combineWithTransferFlags(VkBufferUsageFlags flags, MemBufferTransferDir direction) noexcept {
    switch (direction) {
    case MemBufferTransferDir::SOURCE:
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        break;
    case MemBufferTransferDir::DESTINATION:
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    default:
        break;
    }
    return flags;
}

class MemBuffer {
    VkBuffer hBuffer = VK_NULL_HANDLE;
    VkDeviceMemory hMemory = VK_NULL_HANDLE;
    VkDeviceSize bufSize = 0;

    static MemBuffer create(
        VkPhysicalDevice physicalDevice, //
        Device device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    ) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto buffer = device.createBuffer(bufferInfo);
        if (buffer == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.getHandle(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        VkDeviceMemory memory;
        if (vkAllocateMemory(device.getHandle(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory");
        }

        vkBindBufferMemory(device.getHandle(), buffer, memory, 0);

        return {buffer, memory, size};
    }

public:
    constexpr MemBuffer() noexcept = default;

    constexpr MemBuffer(VkBuffer hBuffer, VkDeviceMemory hMemory, VkDeviceSize bufSize) noexcept : hBuffer(hBuffer), hMemory(hMemory), bufSize(bufSize) {}

    static MemBuffer createVertex(
        VkPhysicalDevice physicalDevice, //
        Device device,
        VkDeviceSize size,
        MemBufferTransferDir direction,
        VkMemoryPropertyFlags properties
    ) {
        auto usage = combineWithTransferFlags(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, direction);
        return MemBuffer::create(physicalDevice, device, size, usage, properties);
    }

    static MemBuffer createIndex(
        VkPhysicalDevice physicalDevice, //
        Device device,
        VkDeviceSize size,
        MemBufferTransferDir direction,
        VkMemoryPropertyFlags properties
    ) {
        auto usage = combineWithTransferFlags(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, direction);
        return MemBuffer::create(physicalDevice, device, size, usage, properties);
    }

    static MemBuffer create(
        VkPhysicalDevice physicalDevice, //
        Device device,
        VkDeviceSize size,
        MemBufferTransferDir direction,
        VkMemoryPropertyFlags properties
    ) {
        auto usage = combineWithTransferFlags(0, direction);
        return MemBuffer::create(physicalDevice, device, size, usage, properties);
    }

    void* mapMemory(Device device) const noexcept {
        void* data;
        vkMapMemory(device.getHandle(), hMemory, 0, bufSize, 0, &data);
        return data;
    }

    void unmapMemory(Device device) const noexcept {
        vkUnmapMemory(device.getHandle(), hMemory);
    }

    VkBuffer buffer() const noexcept {
        return hBuffer;
    }

    VkDeviceMemory memory() const noexcept {
        return hMemory;
    }

    VkDeviceSize size() const noexcept {
        return bufSize;
    }

    void destroy(Device device) const noexcept {
        device.freeMemory(hMemory);
        device.destroyBuffer(hBuffer);
    }
};
