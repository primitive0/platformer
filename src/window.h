#pragma once

#include <cstdint>
#include <vector>

#include "glfw.h"

class Window final {
    GLFWwindow* handle;

public:
    explicit Window(int width, int height, const char* title, bool resizable = false) {
        initGlfw();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (handle == nullptr) {
            throw std::runtime_error("failed to create window");
        }
    }

    void destroy() const noexcept {
        glfwDestroyWindow(handle);
    }

    GLFWwindow* getHandle() const noexcept {
        return handle;
    }

    bool shouldClose() const noexcept {
        return glfwWindowShouldClose(handle);
    }

    VkExtent2D getWindowExtent() const noexcept {
        int width, height;
        glfwGetWindowSize(handle, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    std::vector<const char*> getRequiredVulkanExtensions() const {
        uint32_t count = 0;
        const char** pExtensions = glfwGetRequiredInstanceExtensions(&count);
        std::vector<const char*> extensions(pExtensions, pExtensions + count);
        return extensions;
    }
};
