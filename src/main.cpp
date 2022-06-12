#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "glfw.h"

#include "debug.h"
#include "math/vec.h"
#include "sys/vulkan/device.h"
#include "sys/vulkan/instance.h"
#include "sys/vulkan/mem_buffer.h"
#include "sys/vulkan/pipeline.h"
#include "sys/vulkan/render_pass.h"
#include "sys/vulkan/shaders.h"
#include "sys/vulkan/surface.h"
#include "sys/vulkan/swapchain.h"
#include "util/time.h"
#include "window.h"

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

struct LineVertex {
    Vec2 pos;
    Vec3 color;

    LineVertex(Vec2 pos, Vec3 color) : pos(pos), color(color) {}

    constexpr static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(LineVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    constexpr static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(LineVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(LineVertex, color);

        return attributeDescriptions;
    }
};

struct Vertex {
    Vec2 pos;
    Vec3 color;

    Vertex(Vec2 pos, Vec3 color) : pos(pos), color(color) {}

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

class SolidObject {
public:
    float x0, x1, y0, y1;

    SolidObject(float x0, float x1, float y0, float y1) {
        this->x0 = x0;
        this->x1 = x1;
        this->y0 = y0;
        this->y1 = y1;
    }

    bool collides(const SolidObject& rhs) const noexcept {
        bool xNotCollides = this->x1 < rhs.x0 || this->x0 > rhs.x1;
        bool yNotCollides = this->y1 < rhs.y0 || this->y0 > rhs.y1;
        return !xNotCollides && !yNotCollides;
    }
};

class World {
public:
    std::vector<SolidObject> objects{};
    bool playerOnGround = false;

    float velY = 0.0f;

    World() {
        objects.emplace_back(100, 200, 300, 400);
    }

    void tick(float delta) {
        auto& player = objects[0];
        if (!playerOnGround) {
            addVelocityY(-0.03f);
            player.y0 += velY * delta;
            player.y1 += velY * delta;
        }

        for (auto i = objects.cbegin() + 1; i < objects.cend(); i++) {
            if (player.collides(*i)) {
                playerOnGround = true;
                velY = 0.0f;
            }
        }
    }

    void addVelocityY(float val) {
        const float VELOCITY_MAX = 3.0f;
        velY = std::clamp(this->velY + val, -VELOCITY_MAX, VELOCITY_MAX);
    }
};

class GameRenderer {
    Window window;

    VkInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physicalDevice;
    Device device;

    DeviceQueue graphicsQueue;
    DeviceQueue presentQueue;

    Shaders shaders;
    Shaders shaders1;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    std::vector<Vertex> vertices;

    std::vector<LineVertex> lines;

    MemBuffer vertexBuffer1;
    MemBuffer vertexBuffer;
    MemBuffer indexBuffer;

    SwapChain swapChain;
    RenderPass renderPass;
    GraphicsPipeline graphicsPipeline;
    GraphicsPipeline graphicsPipeline1;

public:
    static GameRenderer initialize(Window window) {
        GameRenderer self;

        self.window = window;

        self.instance = createVulkanInstance(
            VulkanApplicationInfo{
                "Hello Triangle",
                VK_MAKE_VERSION(1, 0, 0),
                "No Engine",
                VK_MAKE_VERSION(1, 0, 0),
            },
            window
        );

        self.surface = createVulkanSurface(self.instance, window);
        self.physicalDevice = findPhysicalDevice(self.instance, self.surface);
        self.device = Device::create(self.physicalDevice);

        self.graphicsQueue = self.device.getDeviceQueue(self.physicalDevice.familyIndices.graphicsFamily);
        self.presentQueue = self.device.getDeviceQueue(self.physicalDevice.familyIndices.presentFamily);

        self.shaders = Shaders::loadShaders(self.device, "vert.spv", "frag.spv");
        self.shaders1 = Shaders::loadShaders(self.device, "line_vert.spv", "line_frag.spv");

        self.commandPool = self.device.createCommandPool(self.physicalDevice.familyIndices.graphicsFamily);
        if (self.commandPool == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to create command pool");
        }

        self.commandBuffer = self.device.allocateCommandBuffer(self.commandPool);
        if (self.commandBuffer == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to allocate command buffer");
        }

        self.imageAvailableSemaphore = self.device.createSemaphore();
        self.renderFinishedSemaphore = self.device.createSemaphore();
        self.inFlightFence = self.device.createFence();
        if (self.imageAvailableSemaphore == VK_NULL_HANDLE || self.renderFinishedSemaphore == VK_NULL_HANDLE || self.inFlightFence == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to create semaphores");
        }

        self.vertices = {};

        self.lines = {
            LineVertex({-1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}), //
            LineVertex({}, {0.0f, 0.0f, 1.0f})};

        self.vertexBuffer1 = MemBuffer::createVertex(
            self.physicalDevice.handle,
            self.device,
            sizeof(LineVertex) * self.lines.size(),
            MemBufferTransferDir::NONE,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        std::array<uint16_t, 6> indices = {0, 1, 2, 2, 3, 0};

        auto stagingBuffer = MemBuffer::create(
            self.physicalDevice.handle, //
            self.device,
            sizeof(uint16_t) * indices.size(),
            MemBufferTransferDir::SOURCE,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        {
            void* data = stagingBuffer.mapMemory(self.device);
            memcpy(data, indices.data(), static_cast<size_t>(stagingBuffer.size()));
            stagingBuffer.unmapMemory(self.device);
        }

        self.indexBuffer = MemBuffer::createIndex(
            self.physicalDevice.handle, //
            self.device,
            stagingBuffer.size(),
            MemBufferTransferDir::DESTINATION,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        copyBuffer(
            self.physicalDevice, //
            self.device,
            self.commandPool,
            self.graphicsQueue.getHandle(),
            stagingBuffer.buffer(),
            self.indexBuffer.buffer(),
            stagingBuffer.size()
        );

        stagingBuffer.destroy(self.device);

        self.recreateSwapChain();

        return self;
    }

    bool render(const World& world) {
        device.waitForFence(inFlightFence);

        vertices.clear();

        int i = 0;
        for (const auto& object : world.objects) {
            auto _x0 = static_cast<float>(object.x0);
            auto _x1 = static_cast<float>(object.x1);
            auto _y0 = static_cast<float>(object.y0);
            auto _y1 = static_cast<float>(object.y1);

            auto x0 = 2.0f / 1000.0f * _x0 - 1.0f;
            auto x1 = 2.0f / 1000.0f * _x1 - 1.0f;
            auto y0 = 1.0f - 2.0f / 1000.0f * _y1;
            auto y1 = 1.0f - 2.0f / 1000.0f * _y0;

            Vec3 fillColor;
            if (i == 0) {
                fillColor = {0.0f, 1.0f, 0.0f};
            } else {
                fillColor = {0.0f, 0.0f, 0.0f};
            }
            auto x0y0 = Vertex({x0, y0}, fillColor);
            auto x1y0 = Vertex({x1, y0}, fillColor);
            auto x1y1 = Vertex({x1, y1}, fillColor);
            auto x0y1 = Vertex({x0, y1}, fillColor);
            vertices.push_back(x0y0);
            vertices.push_back(x1y0);
            vertices.push_back(x1y1);
            vertices.push_back(x0y1);

            i++;
        }

        if (vertexBuffer.buffer() != VK_NULL_HANDLE) {
            vertexBuffer.destroy(device);
        }

        vertexBuffer = MemBuffer::createVertex(
            physicalDevice.handle,
            device,
            sizeof(Vertex) * vertices.size(),
            MemBufferTransferDir::NONE,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        {
            void* data = vertexBuffer.mapMemory(device);
            memcpy(data, vertices.data(), static_cast<size_t>(vertexBuffer.size()));
            vertexBuffer.unmapMemory(device);
        }

        auto extent = window.getWindowExtent();
        auto x = 2.0f / static_cast<float>(extent.width) * static_cast<float>(window.cursor().x) - 1.0f;
        auto y = 2.0f / static_cast<float>(extent.height) * static_cast<float>(window.cursor().y) - 1.0f;

        lines[1].pos.x = x;
        lines[1].pos.y = y;

        {
            void* data = vertexBuffer1.mapMemory(device);
            memcpy(data, lines.data(), static_cast<size_t>(vertexBuffer1.size()));
            vertexBuffer1.unmapMemory(device);
        }

    renderStart:
        VkExtent2D windowExtent = window.getWindowExtent();
        if (windowExtent.width == 0 || windowExtent.height == 0) {
            return false;
        }

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
            recreateSwapChain();
            goto renderStart;
        } else if (!(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)) {
            throw std::runtime_error("failed to recreate swapchain");
        }

        device.resetFence(inFlightFence);

        vkResetCommandBuffer(commandBuffer, 0);
        recordCommandBuffer(imageIndex, world.objects.size());

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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

        graphicsQueue.submit(submitInfo, inFlightFence);

        VkSwapchainKHR swapChains[] = {swapChain.getSwapChainHandle()};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = presentQueue.present(presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasResized()) {
            window.resetResized();
            recreateSwapChain();
            goto renderStart;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        return true;
    }

    void destroy() {
        device.waitIdle();

        graphicsPipeline1.destroy(device);
        graphicsPipeline.destroy(device);
        renderPass.destroy(device);
        swapChain.destroy(device);
        vertexBuffer1.destroy(device);
        vertexBuffer.destroy(device);
        indexBuffer.destroy(device);
        device.destroySemaphore(imageAvailableSemaphore);
        device.destroySemaphore(renderFinishedSemaphore);
        device.destroyFence(inFlightFence);
        device.destroyCommandPool(commandPool);
        shaders1.destroy(device);
        shaders.destroy(device);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        device.destroy();
        vkDestroyInstance(instance, nullptr);
    }

private:
    void recreateSwapChain() {
        device.waitIdle();

        if (swapChain.getSwapChainHandle() != VK_NULL_HANDLE) {
            graphicsPipeline1.destroy(device);
            graphicsPipeline.destroy(device);
            renderPass.destroy(device);
            swapChain.destroy(device);
        }

        swapChain = SwapChain::create(physicalDevice, device, window, surface);
        renderPass = RenderPass::create(device, swapChain);

        {
            auto bindingDescription = Vertex::getBindingDescription();
            auto attributeDescriptions = Vertex::getAttributeDescriptions();
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

            graphicsPipeline = GraphicsPipelineBuilder(device, shaders, swapChain, renderPass)
                                   .withVertexInputStateInfo(vertexInputInfo)
                                   .withInputAssemblyStateInfo(createInputAssemblyStateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST))
                                   .create();
        }

        {
            auto bindingDescription = LineVertex::getBindingDescription();
            auto attributeDescriptions = LineVertex::getAttributeDescriptions();
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

            graphicsPipeline1 = GraphicsPipelineBuilder(device, shaders1, swapChain, renderPass)
                                    .withVertexInputStateInfo(vertexInputInfo)
                                    .withInputAssemblyStateInfo(createInputAssemblyStateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST))
                                    .withRasterizerLineWidth(2.5f)
                                    .create();
        }
    }

    void recordCommandBuffer(uint32_t imageIndex, size_t objectCount) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass.renderPass();
        renderPassInfo.framebuffer = renderPass.getFramebuffer(imageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.getExtent();

        VkClearValue clearColor = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline());

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer(), 0, VK_INDEX_TYPE_UINT16);

        for (int i = 0; i < objectCount; i++) {
            VkBuffer vertexBuffers[] = {vertexBuffer.buffer()};
            VkDeviceSize offsets[] = {sizeof(Vertex) * 4 * i};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
        }

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline1.pipeline());

        {
            VkBuffer vertexBuffers[] = {vertexBuffer1.buffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }

        vkCmdDraw(commandBuffer, 2, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }
};

int main() {
    appStartMs = unixMs();

    setupDebug();

    printVulkanAvailableExtensions();

    Window window = Window::create(800, 600, "Vulkan sample", true);

    if (debugEnabled && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    World world{};
    world.objects.emplace_back(100, 900, 200, 250);
    auto& player = world.objects[0];
    player.y0 += 200;
    player.y1 += 200;

    GameRenderer renderer = GameRenderer::initialize(window);

    bool aKeyPressed = false;
    bool dKeyPressed = false;

    float frameDelta = 0.0f;
    while (!window.shouldClose()) {
        uint64_t frameStart = unixMs();
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
            }

            if (event.action == GLFW_PRESS && event.key == GLFW_KEY_W) {
                wPressed = true;
            }
        }
        window.keyEvents().clear();

        //        auto pressedToString = [](bool pressed) {
        //            return pressed ? "pressed" : "not pressed";
        //        };
        //        std::cout << "a: " << pressedToString(aKeyPressed) << std::endl;
        //        std::cout << "d: " << pressedToString(dKeyPressed) << std::endl;
        if (aKeyPressed && !dKeyPressed) {
            player.x0 -= 2.0f;
            player.x1 -= 2.0f;
        } else if (!aKeyPressed && dKeyPressed) {
            player.x0 += 2.0f;
            player.x1 += 2.0f;
        }

        if (wPressed) {
            world.velY = 2.2f;
            world.playerOnGround = false;
        }

        world.tick(frameDelta);
        renderer.render(world);

        frameDelta = static_cast<float>(unixMs() - frameStart);
    }

    renderer.destroy();

    window.destroy();

    cleanup();

    return EXIT_SUCCESS;
}

//#include <iostream>
//#include <vector>
//
// class AABBRect {
// public:
//    double x0 = 0.0;
//    double x1 = 0.0;
//    double y0 = 0.0;
//    double y1 = 0.0;
//
//    constexpr AABBRect() noexcept = default;
//
//    constexpr AABBRect(double x0, double x1, double y0, double y1) noexcept : x0(x0), x1(x1), y0(y0), y1(y1) {}
//
//    constexpr bool collides(const AABBRect& other) const noexcept {
//        // actually to make this work, we need to assert that x0 <= x1 && y0 <= y1 is true for both
//        bool xNotCollides = this->x1 < other.x0 || this->x0 > other.x1;
//        bool yNotCollides = this->y1 < other.y0 || this->y0 > other.y1;
//        return !xNotCollides && !yNotCollides;
//    }
//};
//
// class World {
//
//};
//
// class WorldRender {
//    WorldRender() = default;
//
// public:
//    virtual void render(const World& world) = 0;
//};
//
// int main() {
//
//}

// class SolidObject {
//     AABBRect hitbox;
//
// public:
//     SolidObject(double x0, double x1, double y0, double y1) : hitbox({x0, x1, y0, y1}) {}
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
//
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
//     bool collides(const SolidObject& object) {
//         return hitbox.collides(object.getHitbox());
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
//             player.getHitbox().y0 -= 0.5;
//             player.getHitbox().y1 -= 0.5;
//         }
//
//         bool onGround = false;
//         for (const auto& object : objects) {
//             if (player.collides(object)) {
//                 auto& playerHitbox = player.getHitbox();
//                 double dY = playerHitbox.y1 - playerHitbox.y0;
//                 playerHitbox.y1 = object.getHitbox().y0;
//                 playerHitbox.y0 = playerHitbox.y1 - dY;
//                 onGround = true;
//             }
//         }
//         player.setOnGround(onGround);
//     }
//
//     void spawnObject(SolidObject object) {
//         objects.push_back(object);
//     }
//
//     void setPlayerPos(double x, double y) noexcept {
//         auto& hitbox = player.getHitbox();
//         double dX = hitbox.x1 - hitbox.x0;
//         double dY = hitbox.y1 - hitbox.y0;
//         hitbox.x0 = x;
//         hitbox.y0 = y;
//         hitbox.x1 = x + dX;
//         hitbox.y1 = y + dY;
//     }
//
//     void printDebugInfo() const {
//         std::cout << player.getHitbox().y0 << std::endl;
//     }
// };
//
// int main() {
//     World world;
//     world.spawnObject({5.0, 10.0, 5.0, 7.0});
//     world.setPlayerPos(7.0, 30.3);
//
//     for (int i = 0; i < 100; i++) {
//         world.tick();
//         world.printDebugInfo();
//     }
// }
