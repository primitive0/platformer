#pragma once

#include <cstdint>
#include <vector>

#include "glfw.h"

void initGlfw();

void terminateGlfw();

class Window final {
    GLFWwindow* handle;

public:
    explicit Window(int width, int height, const char* title) {
        initGlfw();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        this->handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    }

    ~Window() {
        glfwDestroyWindow(this->handle);
    }

    inline GLFWwindow* getHandle() const noexcept {
        return this->handle;
    }

    bool shouldClose() const noexcept {
        return glfwWindowShouldClose(this->handle);
    }

    VkExtent2D getWindowExtent() const noexcept {
        int width, height;
        glfwGetFramebufferSize(this->handle, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
};
