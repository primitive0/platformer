#include "game_renderer.h"

static void copyBuffer(
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

GameRenderer GameRenderer::initialize(Window window) {
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

    self._vkCmdDrawMultiIndexedExt = reinterpret_cast<PFN_vkCmdDrawMultiIndexedEXT>(vkGetDeviceProcAddr(self.device.getHandle(), "vkCmdDrawMultiIndexedEXT"));

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

    self.lines = {};

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

bool GameRenderer::render(const World& world) {
    device.waitForFence(inFlightFence);

    vertices.clear();

    Vec3 fillColor = {0.0f, 1.0f, 0.0f};

    {
        auto object = world.player.aabb();
        auto _x0 = static_cast<float>(object.v0().x);
        auto _x1 = static_cast<float>(object.v1().x);
        auto _y0 = static_cast<float>(object.v0().y);
        auto _y1 = static_cast<float>(object.v1().y);

        auto x0 = 2.0f / 1000.0f * _x0 - 1.0f;
        auto x1 = 2.0f / 1000.0f * _x1 - 1.0f;
        auto y0 = 1.0f - 2.0f / 1000.0f * _y1;
        auto y1 = 1.0f - 2.0f / 1000.0f * _y0;

        auto x0y0 = Vertex({x0, y0}, fillColor);
        auto x1y0 = Vertex({x1, y0}, fillColor);
        auto x1y1 = Vertex({x1, y1}, fillColor);
        auto x0y1 = Vertex({x0, y1}, fillColor);
        vertices.push_back(x0y0);
        vertices.push_back(x1y0);
        vertices.push_back(x1y1);
        vertices.push_back(x0y1);
    }

    fillColor = {0.0f, 0.0f, 0.0f};

    for (const auto& object : world.objects) {
        auto _x0 = static_cast<float>(object.v0().x);
        auto _x1 = static_cast<float>(object.v1().x);
        auto _y0 = static_cast<float>(object.v0().y);
        auto _y1 = static_cast<float>(object.v1().y);

        auto x0 = 2.0f / 1000.0f * _x0 - 1.0f;
        auto x1 = 2.0f / 1000.0f * _x1 - 1.0f;
        auto y0 = 1.0f - 2.0f / 1000.0f * _y1;
        auto y1 = 1.0f - 2.0f / 1000.0f * _y0;

        auto x0y0 = Vertex({x0, y0}, fillColor);
        auto x1y0 = Vertex({x1, y0}, fillColor);
        auto x1y1 = Vertex({x1, y1}, fillColor);
        auto x0y1 = Vertex({x0, y1}, fillColor);
        vertices.push_back(x0y0);
        vertices.push_back(x1y0);
        vertices.push_back(x1y1);
        vertices.push_back(x0y1);
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

    lines.clear();

    fillColor = {0.0f, 0.0f, 1.0f};

    for (const auto& pos : world.posLog) {
        auto _x0 = static_cast<float>(pos.x);
        auto _y0 = static_cast<float>(pos.y);

        auto x0 = 2.0f / 1000.0f * _x0 - 1.0f;
        auto y0 = 1.0f - 2.0f / 1000.0f * _y0;

        lines.emplace_back(Vec2{x0, y0}, fillColor);
    }

    if (!lines.empty()) {
        if (vertexBuffer1.buffer() != VK_NULL_HANDLE) {
            vertexBuffer1.destroy(device);
        }

        vertexBuffer1 = MemBuffer::createVertex(
            physicalDevice.handle,
            device,
            sizeof(LineVertex) * lines.size(),
            MemBufferTransferDir::NONE,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        {
            void* data = vertexBuffer1.mapMemory(device);
            memcpy(data, lines.data(), static_cast<size_t>(vertexBuffer1.size()));
            vertexBuffer1.unmapMemory(device);
        }
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
    recordCommandBuffer(imageIndex, world.objects.size() + 1, world.posLog.size());

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

void GameRenderer::destroy() {
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

void GameRenderer::recreateSwapChain() {
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
                                .withInputAssemblyStateInfo(createInputAssemblyStateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP))
                                .withRasterizerLineWidth(2.5f)
                                .create();
    }
}

void GameRenderer::recordCommandBuffer(uint32_t imageIndex, size_t objectCount, size_t lineCount) {
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

    {
        VkBuffer vertexBuffers[] = {vertexBuffer.buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    }

    std::vector<VkMultiDrawIndexedInfoEXT> draws{};
    for (int i = 0; i < objectCount; i++) {
        VkMultiDrawIndexedInfoEXT info{};
        info.firstIndex = 0;
        info.indexCount = 6;
        info.vertexOffset = 4 * i;
        draws.push_back(info);
    }
    cmdDrawMultiIndexed(commandBuffer, draws.size(), draws.data(), 1, 0, sizeof(VkMultiDrawIndexedInfoEXT), nullptr);

    if (lineCount != 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline1.pipeline());

        VkBuffer vertexBuffers[] = {vertexBuffer1.buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer, lineCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void GameRenderer::cmdDrawMultiIndexed(
    VkCommandBuffer commandBuffer, //
    uint32_t drawCount,
    const VkMultiDrawIndexedInfoEXT* pIndexInfo,
    uint32_t instanceCount,
    uint32_t firstInstance,
    uint32_t stride,
    const int32_t* pVertexOffset
) {
    _vkCmdDrawMultiIndexedExt(commandBuffer, drawCount, pIndexInfo, instanceCount, firstInstance, stride, pVertexOffset);
}
