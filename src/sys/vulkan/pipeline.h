#pragma once

#include <vector>

#include "render_pass.h"
#include "shaders.h"
#include "swapchain.h"

struct GraphicsPipelineCreateInfo {
    Device device{};
    Shaders shaders{};
    SwapChain* swapChain = nullptr;
    RenderPass* renderPass = nullptr;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};

    float lineWidth = 1.0f;

    GraphicsPipelineCreateInfo() noexcept {
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    }
};

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
    rasterizer.lineWidth = pipelineCreateInfo.lineWidth;
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
    vkPipelineInfo.pInputAssemblyState = &pipelineCreateInfo.inputAssemblyStateCreateInfo;
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

class GraphicsPipeline {
    VkPipelineLayout hPipelineLayout;
    VkPipeline hPipeline;

public:
    GraphicsPipeline() : hPipelineLayout(VK_NULL_HANDLE), hPipeline(VK_NULL_HANDLE) {}

    GraphicsPipeline(VkPipelineLayout pipelineLayout, VkPipeline pipeline) : hPipelineLayout(pipelineLayout), hPipeline(pipeline) {}

    VkPipelineLayout pipelineLayout() const noexcept {
        return hPipelineLayout;
    }

    VkPipeline pipeline() const noexcept {
        return hPipeline;
    }
    void destroy(Device device) const noexcept {
        vkDestroyPipeline(device.getHandle(), hPipeline, nullptr);
        vkDestroyPipelineLayout(device.getHandle(), hPipelineLayout, nullptr);
    }
};

class GraphicsPipelineBuilder {
    GraphicsPipelineCreateInfo info{};

public:
    GraphicsPipelineBuilder(Device device, Shaders shaders, SwapChain& swapChain, RenderPass& renderPass) noexcept {
        info.device = device;
        info.shaders = shaders;
        info.swapChain = &swapChain;
        info.renderPass = &renderPass;
    };

    GraphicsPipelineBuilder withVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo) noexcept {
        info.vertexInputStateCreateInfo = vertexInputStateCreateInfo;
        return *this;
    }

    GraphicsPipelineBuilder withInputAssemblyStateInfo(VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo) noexcept {
        info.inputAssemblyStateCreateInfo = inputAssemblyStateCreateInfo;
        return *this;
    }

    GraphicsPipelineBuilder withRasterizerLineWidth(float lineWidth) noexcept {
        info.lineWidth = lineWidth;
        return *this;
    }

    GraphicsPipeline create() const {
        auto pipelineLayout = createPipelineLayout(info.device.getHandle());
        auto pipeline = createVkPipeline(info, info.renderPass->renderPass(), pipelineLayout);
        return {pipelineLayout, pipeline};
    }
};

inline VkPipelineInputAssemblyStateCreateInfo createInputAssemblyStateInfo(VkPrimitiveTopology topology) noexcept {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return inputAssembly;
}
