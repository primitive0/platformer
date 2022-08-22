#pragma once

#include <cstdint>
#include <vector>

#include "glfw.h"

class KeyPressEvent {
public:
    int key;
    int scancode;
    int action;
    int mods;

    KeyPressEvent(int key, int scancode, int action, int mods) : key(key), scancode(scancode), action(action), mods(mods) {}
};

class WindowData {
public:
    bool resized = false;
    std::vector<KeyPressEvent> keyPressEvents;

    double cursorPosX = 0.0;
    double cursorPosY = 0.0;

    void emitKeyPress(int key, int scancode, int action, int mods) {
        if (keyPressEvents.size() > 50) { // TODO: fix
            return;
        }
        keyPressEvents.emplace_back(key, scancode, action, mods);
    }
};

inline void framebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept {
    auto* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
    data->resized = true;
}

inline void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept {
    auto* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
    data->emitKeyPress(key, scancode, action, mods);
}

inline void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
    auto* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
    data->cursorPosX = xPos;
    data->cursorPosY = yPos;
}

class CursorCoords {
public:
    double x, y;
};

class Window {
    GLFWwindow* handle;
    WindowData* data;

public:
    Window() : handle(nullptr), data(nullptr) {}

    Window(const Window&) = default;

    Window(GLFWwindow* handle, WindowData* data) noexcept : handle(handle), data(data) {}

    static Window create(int width, int height, const char* title, bool resizable = false) {
        auto data = new WindowData;

        initGlfw();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        auto handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (handle == nullptr) {
            throw std::runtime_error("failed to create window");
        }

        glfwSetWindowUserPointer(handle, data);
        glfwSetFramebufferSizeCallback(handle, framebufferSizeCallback);
        glfwSetKeyCallback(handle, keyCallback);
        glfwSetCursorPosCallback(handle, cursorPosCallback);

        return {handle, data};
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

    bool wasResized() const noexcept {
        return data != nullptr && data->resized;
    }

    void resetResized() noexcept {
        if (data != nullptr) {
            data->resized = false;
        }
    }

    std::vector<KeyPressEvent>& keyEvents() {
        if (data == nullptr) {
            throw std::runtime_error("window data is not initialized");
        }
        return data->keyPressEvents;
    }

    CursorCoords cursor() {
        if (data == nullptr) {
            return {};
        }
        return {data->cursorPosX, data->cursorPosY};
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

    VkSurfaceKHR createVkSurface(VkInstance vkInstance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(vkInstance, handle, nullptr, &surface) != VK_SUCCESS || surface == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to create vulkan surface");
        }
        return surface;
    }
};
