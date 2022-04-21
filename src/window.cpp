#include "window.h"

#include <cstdlib>

bool glfwInitialized = false;

void initGlfw() {
    if (glfwInitialized) {
        return;
    }

    if (glfwInit() == GLFW_TRUE) {
        glfwInitialized = true;
    } else {
        std::exit(1); // TODO: replace
    }
}

void terminateGlfw() {
    if (!glfwInitialized) {
        return;
    }

    glfwTerminate();
}
