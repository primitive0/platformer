#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#undef GLFW_INCLUDE_VULKAN

extern bool glfwInitialized;

enum class GlfwInitializeResult {
    ALREADY_INIT,
    INITIALIZED,
    FAILED_TO_INIT,
};

inline GlfwInitializeResult initGlfw() {
    if (glfwInitialized) {
        return GlfwInitializeResult::ALREADY_INIT;
    }

    if (glfwInit() == GLFW_TRUE) {
        glfwInitialized = true;
        return GlfwInitializeResult::INITIALIZED;
    } else {
        return GlfwInitializeResult::FAILED_TO_INIT;
    }
}

inline void terminateGlfw() {
    if (glfwInitialized) {
        glfwTerminate();
    }
}
