#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sys/time.h>

#include "glfw.h"

#include "debug.h"
#include "helper/vulkan/extensions.h"
#include "math/vec.h"
#include "sys/vulkan/device.h"
#include "sys/vulkan/instance.h"
#include "sys/vulkan/mem_buffer.h"
#include "sys/vulkan/pipeline.h"
#include "sys/vulkan/shaders.h"
#include "sys/vulkan/surface.h"
#include "sys/vulkan/swapchain.h"
#include "window.h"

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

struct Vertex {
    Vec2 pos;
    Vec3 color;

    constexpr static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    constexpr static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

void recordCommandBuffer(
    VkPipeline graphicsPipeline,
    VkExtent2D swapChainExtent,
    const std::vector<VkFramebuffer>& framebuffers,
    VkRenderPass renderPass,
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    VkBuffer vertexBuffer,
    VkBuffer indexBuffer
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

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

bool framebufferResized = false;

ulong unixMs() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void copyBuffer(
    const PhysicalDevice& physicalDevice, //
    Device device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size
) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device.getHandle(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device.getHandle(), commandPool, 1, &commandBuffer);
}

int main() {
    auto start = unixMs();

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

    auto commandPool = device.createCommandPool(physicalDevice.familyIndices.graphicsFamily);
    if (commandPool == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to create command pool");
    }

    auto commandBuffer = device.allocateCommandBuffer(commandPool);
    if (commandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to allocate command buffer");
    }

    auto imageAvailableSemaphore = device.createSemaphore();
    auto renderFinishedSemaphore = device.createSemaphore();
    auto inFlightFence = device.createFence();
    if (imageAvailableSemaphore == VK_NULL_HANDLE || renderFinishedSemaphore == VK_NULL_HANDLE || inFlightFence == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to create semaphores");
    }

    std::array<Vertex, 4> vertices = {
        Vertex({{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}),
        Vertex({{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}),
        Vertex({{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}),
        Vertex({{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}),
    };

    std::array<uint16_t, 6> indices = {0, 1, 2, 2, 3, 0};

    auto stagingBuffer = MemBuffer::create(
        physicalDevice.handle, //
        device,
        sizeof(uint16_t) * indices.size(),
        MemBufferTransferDir::SOURCE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    {
        void* data = stagingBuffer.mapMemory(device);
        memcpy(data, indices.data(), static_cast<size_t>(stagingBuffer.size()));
        stagingBuffer.unmapMemory(device);
    }

    auto indexBuffer = MemBuffer::createIndex(
        physicalDevice.handle, //
        device,
        stagingBuffer.size(),
        MemBufferTransferDir::DESTINATION,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    copyBuffer(physicalDevice, device, commandPool, graphicsQueue, stagingBuffer.buffer(), indexBuffer.buffer(), stagingBuffer.size());

    stagingBuffer.destroy(device);

    auto vertexBuffer = MemBuffer::createVertex(
        physicalDevice.handle,
        device,
        sizeof(Vertex) * vertices.size(),
        MemBufferTransferDir::NONE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    SwapChain swapChain;
    GraphicsPipeline graphicsPipeline;

createSwapChain:
    VkExtent2D windowExtent = window.getWindowExtent();
    while (windowExtent.width == 0 || windowExtent.height == 0) {
        glfwWaitEvents();
        windowExtent = window.getWindowExtent();
    }

    device.waitIdle();

    if (swapChain.getSwapChainHandle() != VK_NULL_HANDLE) {
        graphicsPipeline.destroy(device);
        swapChain.destroy(device);
    }

    swapChain = SwapChain::create(physicalDevice, device, window, surface);

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    graphicsPipeline = GraphicsPipelineBuilder(device, shaders, swapChain).withVertexInputStateInfo(vertexInputInfo).create();

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

        auto elapsed = static_cast<double>(unixMs() - start);
        auto y = static_cast<float>(sin(elapsed * 0.001));
        y = std::abs(y);
        vertices[0].color = {y, 0.0f, 0.0f};
        vertices[1].color = {0.0f, y, 0.0f};
        vertices[2].color = {0.0f, 0.0f, y};
        vertices[3].color = {y, y, y};

        void* data = vertexBuffer.mapMemory(device);
        memcpy(data, vertices.data(), static_cast<size_t>(vertexBuffer.size()));
        vertexBuffer.unmapMemory(device);

        vkResetCommandBuffer(commandBuffer, 0);
        recordCommandBuffer(
            graphicsPipeline.getPipelineHandle(),
            swapChain.getExtent(),
            graphicsPipeline.getFramebuffers(),
            graphicsPipeline.getRenderPass(),
            commandBuffer,
            imageIndex,
            vertexBuffer.buffer(),
            indexBuffer.buffer()
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
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);
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
