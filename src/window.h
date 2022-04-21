#pragma once

#include <cstdint>

#include <GLFW/glfw3.h>

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

    inline GLFWwindow* getHandle() {
        return this->handle;
    }

    bool shouldClose() {
        return glfwWindowShouldClose(this->handle);
    }
};
