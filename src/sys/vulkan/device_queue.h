#pragma once

#include <stdexcept>

#include "../../glfw.h"

class DeviceQueue {
    VkQueue handle;

public:
    DeviceQueue() : handle(VK_NULL_HANDLE) {}

    explicit DeviceQueue(VkQueue handle) : handle(handle) {}

    VkQueue getHandle() const noexcept {
        return handle;
    }

    void submit(const VkSubmitInfo& info, VkFence fence) const noexcept {
        VkResult result = vkQueueSubmit(handle, 1, &info, fence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer");
        }
    }

    VkResult present(const VkPresentInfoKHR& presentInfo) {
        return vkQueuePresentKHR(handle, &presentInfo);
    }
};
