#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "glfw.h"

#include "debug.h"
#include "game/world.h"
#include "game_renderer.h"
#include "platform/time.h"
#include "sys/vulkan/instance.h"
#include "window.h"

#undef min
#undef max

// TODO: refactor physics
// TODO: add rendering fallback if multidraw is not available

const float PHYSICS_SUBSTEP_DELTA_MAX = 0.24;

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

        World world{};
        world.objects.emplace_back(100, 900, 200, 250);
        auto& player = world.player;
        player.pos() = {150.0f, 300.0f};
        world.player.vel() = {0.8f, 2.5f};

        std::cout << "initializing renderer" << std::endl;

        GameRenderer renderer = GameRenderer::initialize(window);

        std::cout << "renderer initialized" << std::endl;

        bool aKeyPressed = false;
        bool dKeyPressed = false;

        uint64_t lFrameDelta = 0;
        uint64_t frameCount = 0;
        uint64_t gameLoopStart = unixUsecs();
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
                    world = World{};
                    world.objects.emplace_back(100, 900, 200, 250);
                    player.pos() = {150.0f, 300.0f};
                    world.player.vel() = {0.8f, 2.5f};
                } else if (event.key == GLFW_KEY_1 && event.action == GLFW_PRESS) {
                    player.vel() = {-0.8f, 2.5f};
                    world.player.setOnGround(false);
                } else if (event.key == GLFW_KEY_2 && event.action == GLFW_PRESS) {
                    player.vel() = {0.8f, 2.5f};
                    world.player.setOnGround(false);
                }

                if (event.action == GLFW_PRESS && event.key == GLFW_KEY_W) {
                    wPressed = true;
                }
            }
            window.keyEvents().clear();

            float fFrameDelta = static_cast<float>(lFrameDelta) / 1000.0f;

            if (wPressed) {
                world.player.vel().y = 2.5f;
                world.player.setOnGround(false);
            }

            if (player.vel() == VEC2_ZEROED) {
                std::cout << "x: " << player.pos().x << "   y: " << player.pos().y << std::endl;
                world = World{};
                world.objects.emplace_back(100, 900, 200, 250);
                player.pos() = {150.0f, 300.0f};
                world.player.vel() = {0.8f, 2.5f};
            }

            if (fFrameDelta > PHYSICS_SUBSTEP_DELTA_MAX) {
                float fSubsteps = fFrameDelta / PHYSICS_SUBSTEP_DELTA_MAX;
                float intSubsteps = std::trunc(fSubsteps);
                float leftSubsteps = fSubsteps - intSubsteps;

                auto iSubsteps = static_cast<uint32_t>(intSubsteps);
                for (uint32_t i = 0; i < iSubsteps; i++) {
                    if (aKeyPressed && !dKeyPressed) {
                        world.addVelocityX(-0.011f * PHYSICS_SUBSTEP_DELTA_MAX, 0.8f);
                    } else if (!aKeyPressed && dKeyPressed) {
                        world.addVelocityX(0.011f * PHYSICS_SUBSTEP_DELTA_MAX, 0.8f);
                    } else if (world.player.isOnGround()) {
                        world.slowDown(0.005f * PHYSICS_SUBSTEP_DELTA_MAX);
                    }

                    world.tick(PHYSICS_SUBSTEP_DELTA_MAX);
                }

                if (aKeyPressed && !dKeyPressed) {
                    world.addVelocityX(-0.011f * leftSubsteps, 0.8f);
                } else if (!aKeyPressed && dKeyPressed) {
                    world.addVelocityX(0.011f * leftSubsteps, 0.8f);
                } else if (world.player.isOnGround()) {
                    world.slowDown(0.005f * leftSubsteps);
                }

                world.tick(leftSubsteps);
            } else {
                if (aKeyPressed && !dKeyPressed) {
                    world.addVelocityX(-0.011f * fFrameDelta, 0.8f);
                } else if (!aKeyPressed && dKeyPressed) {
                    world.addVelocityX(0.011f * fFrameDelta, 0.8f);
                } else if (world.player.isOnGround()) {
                    world.slowDown(0.005f * fFrameDelta);
                }

                world.tick(fFrameDelta);
            }

            //            auto extent = window.getWindowExtent();
            //            auto cursorX = 1000.0f / static_cast<float>(extent.width) * static_cast<float>(window.cursor().x);
            //            auto cursorY = 1000.0f - 1000.0f / static_cast<float>(extent.height) * static_cast<float>(window.cursor().y);

            renderer.render(world);

            //            Sleep(180);

            lFrameDelta = unixUsecs() - frameStart;
            frameCount++;
        }

        std::cout << std::endl;

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
