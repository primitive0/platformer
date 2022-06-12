#pragma once

#include "device.h"

class RenderPass {
    VkRenderPass hRenderPass;
    std::vector<VkFramebuffer> framebuffers;

public:
    RenderPass() : hRenderPass(VK_NULL_HANDLE), framebuffers() {}

    RenderPass(VkRenderPass renderPass, std::vector<VkFramebuffer>&& framebuffers) : hRenderPass(renderPass), framebuffers(std::move(framebuffers)) {}

    static RenderPass create(Device device, const SwapChain& swapChain) {
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
        if (vkCreateRenderPass(device.getHandle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass");
        }

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

            if (vkCreateFramebuffer(device.getHandle(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer");
            }
        }

        return {renderPass, std::move(framebuffers)};
    }

    VkRenderPass renderPass() const noexcept {
        return hRenderPass;
    }

    VkFramebuffer getFramebuffer(size_t i) const noexcept {
        return framebuffers[i];
    }

    void destroy(Device device) const {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device.getHandle(), framebuffer, nullptr);
        }
        vkDestroyRenderPass(device.getHandle(), hRenderPass, nullptr);
    }
};
