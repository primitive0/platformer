#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "glfw.h"

#include "debug.h"
#include "game/world.h"
#include "game/game.h"
#include "game_renderer.h"
#include "platform/time.h"
#include "sys/vulkan/instance.h"
#include "window.h"

// TODO: refactor physics
// TODO: add rendering fallback if multidraw is not available

static uint64_t appStartMs;

void cleanup() {
    terminateGlfw();
}

void printVulkanAvailableExtensions() {
    auto vkExtensions = enumerateInstanceExtensionProperties();

    std::cout << "available extensions:\n";

    for (const auto& extension : vkExtensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

int main() {
    try {
        appStartMs = unixUsecs();

        setupDebug();

        printVulkanAvailableExtensions();

        Window window = Window::create(800, 600, "Vulkan sample", true);

        if (debugEnabled && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        Game game{};
        auto& player = game.world.player;

        std::cout << "initializing renderer" << std::endl;

        GameRenderer renderer = GameRenderer::initialize(window);

        std::cout << "renderer initialized" << std::endl;

        bool aKeyPressed = false;
        bool dKeyPressed = false;

        uint64_t lFrameDelta = 0;
        uint64_t frameCount = 0;
        uint64_t gameLoopStart = unixUsecs();
        uint64_t gameStart = gameLoopStart;
        float frameDeltas = 0.0f;
//        uint32_t substeps = 0;
        while (!window.shouldClose()) {
            uint64_t frameStart = unixUsecs();
            glfwPollEvents();

            bool wPressed = false;
            for (const auto& event : window.keyEvents()) {
                if (event.key == GLFW_KEY_A) {
                    if (event.action == GLFW_PRESS) {
                        aKeyPressed = true;
                    } else if (event.action == GLFW_RELEASE) {
                        aKeyPressed = false;
                    }
                } else if (event.key == GLFW_KEY_D) {
                    if (event.action == GLFW_PRESS) {
                        dKeyPressed = true;
                    } else if (event.action == GLFW_RELEASE) {
                        dKeyPressed = false;
                    }
                } else if (event.key == GLFW_KEY_X && event.action == GLFW_PRESS) {
                    game = Game{};
                }

                if (event.action == GLFW_PRESS && event.key == GLFW_KEY_W) {
                    wPressed = true;
                }
            }
            window.keyEvents().clear();

            float fFrameDelta = static_cast<float>(lFrameDelta) / 1000.0f;

            if (player.vel() == VEC2_ZEROED) {
                std::cout << "time passed: " << static_cast<float>((unixUsecs() - gameStart) / 1000) / 1000.0f << std::endl;
                double fps = static_cast<double>(frameCount) / (static_cast<double>(unixUsecs() - gameLoopStart) / (1000.0 * 1000.0f));
                std::cout << "avg fps: " << fps << std::endl;
                std::cout << "avg frame delta: " << static_cast<double>(frameDeltas) / static_cast<double>(frameCount) << std::endl;
                gameStart = unixUsecs();
                gameLoopStart = unixUsecs();
                frameCount = 0;
                frameDeltas = 0.0f;
                game = Game{};
            }

            if (wPressed) {
                game.playerJump();
            }
            game.moveLeft = aKeyPressed;
            game.moveRight = dKeyPressed;

            game.process(fFrameDelta);
            renderer.render(game.world);

            frameDeltas += fFrameDelta;

            lFrameDelta = unixUsecs() - frameStart;
            frameCount++;
        }

        std::cout << player.pos().x << " " << player.pos().y << std::endl;
        std::cout << static_cast<double>(frameCount) / (static_cast<double>(unixUsecs() - gameLoopStart) / (1000.0 * 1000.0f)) << std::endl;

        renderer.destroy();

        window.destroy();

        cleanup();

        return EXIT_SUCCESS;
    } catch (std::exception& exception) {
        std::cout << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
}
