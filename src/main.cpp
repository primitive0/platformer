#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "glfw.h"

#include "debug.h"
#include "helper/vulkan/extensions.h"
#include "sys/vulkan/device.h"
#include "sys/vulkan/instance.h"
#include "sys/vulkan/pipeline.h"
#include "sys/vulkan/shaders.h"
#include "sys/vulkan/surface.h"
#include "sys/vulkan/swapchain.h"
#include "window.h"

void cleanup() { terminateGlfw(); }

void printVulkanAvailableExtensions() {
    auto vkExtensions = enumerateInstanceExtensionProperties();

    std::cout << "available extensions:\n";

    for (const auto& extension : vkExtensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

void recordCommandBuffer(
    VkPipeline graphicsPipeline,
    VkExtent2D swapChainExtent,
    const std::vector<VkFramebuffer>& framebuffers,
    VkRenderPass renderPass,
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex
) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

bool framebufferResized = false;

int main() {
    setupDebug();

    printVulkanAvailableExtensions();

    Window window(800, 600, "Vulkan sample", true);

    glfwSetFramebufferSizeCallback(window.getHandle(), [](GLFWwindow* window, int width, int height) {
        framebufferResized = true;
    });

    if (debugEnabled && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    auto instance = createVulkanInstance(
        VulkanApplicationInfo{
            "Hello Triangle",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
        },
        window
    );

    auto surface = createVulkanSurface(instance, window);
    auto physicalDevice = findPhysicalDevice(instance, surface);
    auto device = Device::create(physicalDevice);

    VkQueue graphicsQueue = device.getDeviceQueue(physicalDevice.familyIndices.graphicsFamily);
    VkQueue presentQueue = device.getDeviceQueue(physicalDevice.familyIndices.presentFamily);

    auto shaders = Shaders::loadShaders(device);

    VkCommandPool commandPool = device.createCommandPool(physicalDevice.familyIndices.graphicsFamily);
    if (commandPool == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to create command pool");
    }

    VkCommandBuffer commandBuffer = device.allocateCommandBuffer(commandPool);
    if (commandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to allocate command buffer");
    }

    VkSemaphore imageAvailableSemaphore = device.createSemaphore();
    VkSemaphore renderFinishedSemaphore = device.createSemaphore();
    VkFence inFlightFence = device.createFence();
    if (imageAvailableSemaphore == VK_NULL_HANDLE || renderFinishedSemaphore == VK_NULL_HANDLE || inFlightFence == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to create semaphores");
    }

    SwapChain swapChain;
    GraphicsPipeline graphicsPipeline;

createSwapChain:
    device.waitIdle();

    if (swapChain.getSwapChainHandle() != VK_NULL_HANDLE) {
        graphicsPipeline.destroy(device);
        swapChain.destroy(device);
    }

    swapChain = SwapChain::create(physicalDevice, device, window, surface);
    graphicsPipeline = GraphicsPipeline::create(device, shaders, swapChain);

    while (!window.shouldClose()) {
        glfwPollEvents();

        vkWaitForFences(device.getHandle(), 1, &inFlightFence, VK_TRUE, NUM_MAX<uint64_t>);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            device.getHandle(),
            swapChain.getSwapChainHandle(),
            NUM_MAX<uint64_t>,
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
        ); //
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            goto createSwapChain;
        } else if (!(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)) {
            throw std::runtime_error("failed to recreate swapchain");
        }

        vkResetFences(device.getHandle(), 1, &inFlightFence);

        vkResetCommandBuffer(commandBuffer, 0);
        recordCommandBuffer(
            graphicsPipeline.getPipelineHandle(),
            swapChain.getExtent(),
            graphicsPipeline.getFramebuffers(),
            graphicsPipeline.getRenderPass(),
            commandBuffer,
            imageIndex
        );

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer");
        }

        VkSwapchainKHR swapChains[] = {swapChain.getSwapChainHandle()};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            goto createSwapChain;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    device.waitIdle();

    graphicsPipeline.destroy(device);
    swapChain.destroy(device);
    device.destroySemaphore(imageAvailableSemaphore);
    device.destroySemaphore(renderFinishedSemaphore);
    device.destroyFence(inFlightFence);
    device.destroyCommandPool(commandPool);
    shaders.destroy(device);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    device.destroy();
    vkDestroyInstance(instance, nullptr);

    window.destroy();

    cleanup();

    return EXIT_SUCCESS;
}

// struct AABBRect {
//     double xMin;
//     double yMin;
//     double xMax;
//     double yMax;
//
//     bool checkCollision(const AABBRect& other) const noexcept {
//         return !(this->xMax < other.xMin || other.xMax < this->xMin)
//                && !(this->yMax < other.yMin || other.yMax < this->yMin);
//     }
// };
//
// class SolidObject {
//     AABBRect hitbox;
//
// public:
//     SolidObject(double xMin, double yMin, double xMax, double yMax) : hitbox({xMin, yMin, xMax, yMax}) {}
//
//     const AABBRect& getHitbox() const noexcept {
//         return hitbox;
//     }
//
//     AABBRect& getHitbox() noexcept {
//         return hitbox;
//     }
// };
//
// class Player {
//     AABBRect hitbox;
//     bool onGround = false;
// public:
//     Player() : hitbox({0.0, 0.0, 1.0, 1.0}) {}
//
//     const AABBRect& getHitbox() const noexcept {
//         return hitbox;
//     }
//
//     AABBRect& getHitbox() noexcept {
//         return hitbox;
//     }
//
//     bool isOnGround() const noexcept {
//         return onGround;
//     }
//
//     void setOnGround(bool val) noexcept {
//         onGround = val;
//     }
//
//     bool checkCollision(const SolidObject& object) {
//         return hitbox.checkCollision(object.getHitbox());
//     }
// };
//
// class World {
//     Player player;
//     std::vector<SolidObject> objects;
//
// public:
//     void tick() noexcept {
//         if (!player.isOnGround()) {
//             player.getHitbox().yMin += 0.5;
//             player.getHitbox().yMax += 0.5;
//         }
//
//         for (const auto& object : objects) {
//             if (player.checkCollision(object)) {
//                 auto& playerHitbox = player.getHitbox();
//                 double dY = playerHitbox.yMax - playerHitbox.yMin;
//                 playerHitbox.yMax = object.getHitbox().yMin;
//                 playerHitbox.yMin = playerHitbox.yMax - dY;
//                 player.setOnGround(true);
//             }
//         }
//     }
//
//     void spawnObject(SolidObject object) {
//         objects.push_back(object);
//     }
//
//     void setPlayerPos(double x, double y) noexcept {
//         auto& hitbox = player.getHitbox();
//         double dX = hitbox.xMax - hitbox.xMin;
//         double dY = hitbox.yMax - hitbox.yMin;
//         hitbox.xMin = x;
//         hitbox.yMin = y;
//         hitbox.xMax = x + dX;
//         hitbox.yMax = y + dY;
//     }
//
//     void printDebugInfo() const {
//         std::cout << player.getHitbox().yMin << std::endl;
//     }
// };
//
// int main() {
//     World world;
//     world.spawnObject({5.0, 5.0, 10.0, 10.0});
//     world.setPlayerPos(7.0, -30.3);
//
//     for (int i = 0; i < 100; i++) {
//         world.tick();
//         world.printDebugInfo();
//     }
// }
