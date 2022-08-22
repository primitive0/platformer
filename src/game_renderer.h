#pragma once

#include "glfw.h"

#include "debug.h"
#include "game/AABB.h"
#include "game/player.h"
#include "game/world.h"
#include "math/vec.h"
#include "platform/thread.h"
#include "platform/time.h"
#include "sys/vulkan/device.h"
#include "sys/vulkan/instance.h"
#include "sys/vulkan/mem_buffer.h"
#include "sys/vulkan/pipeline.h"
#include "sys/vulkan/render_pass.h"
#include "sys/vulkan/shaders.h"
#include "sys/vulkan/surface.h"
#include "sys/vulkan/swapchain.h"
#include "window.h"

struct Vertex {
    Vec2 pos;
    Vec3 color;

    Vertex(Vec2 pos, Vec3 color) : pos(pos), color(color) {}

    static constexpr VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
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

class GameRenderer {
    Window window;

    VkInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physicalDevice;
    Device device;

    PFN_vkCmdDrawMultiIndexedEXT _vkCmdDrawMultiIndexedExt;

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
    std::vector<Vertex> lines;

    MemBuffer vertexBuffer1;
    MemBuffer vertexBuffer;
    MemBuffer indexBuffer;

    SwapChain swapChain;
    RenderPass renderPass;
    GraphicsPipeline graphicsPipeline;
    GraphicsPipeline graphicsPipeline1;

public:
    static GameRenderer initialize(Window window);

    bool render(const World& world);

    void destroy();

private:
    void recreateSwapChain();

    void recordCommandBuffer(uint32_t imageIndex, size_t objectCount, size_t lineCount);

    void cmdDrawMultiIndexed(
        VkCommandBuffer commandBuffer, //
        uint32_t drawCount,
        const VkMultiDrawIndexedInfoEXT* pIndexInfo,
        uint32_t instanceCount,
        uint32_t firstInstance,
        uint32_t stride,
        const int32_t* pVertexOffset
    );
};
