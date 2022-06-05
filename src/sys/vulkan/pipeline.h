#pragma once

#include <vector>

#include "shaders.h"
#include "swapchain.h"

struct GraphicsPipelineCreateInfo {
    Device device{};
    Shaders shaders{};
    SwapChain* swapChain = nullptr;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};

    GraphicsPipelineCreateInfo() noexcept {
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    }
};

inline VkRenderPass createRenderPass(VkDevice device, const SwapChain& swapChain) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.getSurfaceFormat().format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }

    return renderPass;
}

inline VkPipelineLayout createPipelineLayout(VkDevice device) {
    VkPipelineLayout pipelineLayout;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    return pipelineLayout;
}

inline VkPipeline createVkPipeline(const GraphicsPipelineCreateInfo& pipelineCreateInfo, VkRenderPass renderPass, VkPipelineLayout pipelineLayout) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = pipelineCreateInfo.shaders.vertShader;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = pipelineCreateInfo.shaders.fragShader;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(pipelineCreateInfo.swapChain->getExtent().width);
    viewport.height = static_cast<float>(pipelineCreateInfo.swapChain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = pipelineCreateInfo.swapChain->getExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    const std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo vkPipelineInfo{};
    vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vkPipelineInfo.stageCount = 2;
    vkPipelineInfo.pStages = shaderStages;
    vkPipelineInfo.pVertexInputState = &pipelineCreateInfo.vertexInputStateCreateInfo;
    vkPipelineInfo.pInputAssemblyState = &inputAssembly;
    vkPipelineInfo.pViewportState = &viewportState;
    vkPipelineInfo.pRasterizationState = &rasterizer;
    vkPipelineInfo.pMultisampleState = &multisampling;
    vkPipelineInfo.pColorBlendState = &colorBlending;
    vkPipelineInfo.layout = pipelineLayout;
    vkPipelineInfo.renderPass = renderPass;
    vkPipelineInfo.subpass = 0;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(pipelineCreateInfo.device.getHandle(), VK_NULL_HANDLE, 1, &vkPipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    return graphicsPipeline;
}

inline std::vector<VkFramebuffer> createFramebuffers(VkDevice device, VkRenderPass renderPass, const SwapChain& swapChain) {
    const auto& imageViews = swapChain.getImageViews();

    std::vector<VkFramebuffer> framebuffers;
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChain.getExtent().width;
        framebufferInfo.height = swapChain.getExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }

    return framebuffers;
}

class GraphicsPipeline {
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> framebuffers;

public:
    GraphicsPipeline() noexcept : renderPass(), pipelineLayout(), pipeline(), framebuffers() {}

    GraphicsPipeline(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline pipeline, std::vector<VkFramebuffer>&& framebuffers) noexcept
        : renderPass(renderPass), pipelineLayout(pipelineLayout), pipeline(pipeline), framebuffers(std::move(framebuffers)) {}

    VkRenderPass getRenderPass() const noexcept {
        return renderPass;
    }

    VkPipelineLayout getPipelineLayout() const noexcept {
        return pipelineLayout;
    }

    VkPipeline getPipelineHandle() const noexcept {
        return pipeline;
    }

    const std::vector<VkFramebuffer>& getFramebuffers() const noexcept {
        return framebuffers;
    }

    void destroy(Device device) const noexcept {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device.getHandle(), framebuffer, nullptr);
        }
        vkDestroyPipeline(device.getHandle(), pipeline, nullptr);
        vkDestroyPipelineLayout(device.getHandle(), pipelineLayout, nullptr);
        vkDestroyRenderPass(device.getHandle(), renderPass, nullptr);
    }
};

class GraphicsPipelineBuilder {
    GraphicsPipelineCreateInfo info{};

public:
    GraphicsPipelineBuilder(Device device, Shaders shaders, SwapChain& swapChain) noexcept {
        info.device = device;
        info.shaders = shaders;
        info.swapChain = &swapChain;
    };

    GraphicsPipelineBuilder withVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo) noexcept {
        info.vertexInputStateCreateInfo = vertexInputStateCreateInfo;
        return *this;
    }

    GraphicsPipeline create() const {
        auto renderPass = createRenderPass(info.device.getHandle(), *info.swapChain);
        auto pipelineLayout = createPipelineLayout(info.device.getHandle());
        auto pipeline = createVkPipeline(info, renderPass, pipelineLayout);
        auto framebuffers = createFramebuffers(info.device.getHandle(), renderPass, *info.swapChain);
        return {renderPass, pipelineLayout, pipeline, std::move(framebuffers)};
    }
};
