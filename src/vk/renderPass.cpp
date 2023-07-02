/// @file      renderPass.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vk/device.hpp>
#include <vk/renderPass.hpp>
#include <vk/swapchain.hpp>

using namespace rayce;

RenderPass::RenderPass(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<Swapchain>& swapchain, const VkAttachmentLoadOp colorBufferLoadOp)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    // attachment(s)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapchain->getSurfaceFormat().format;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = colorBufferLoadOp;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = colorBufferLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // subpass description
    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = 1;
    subpassDescription.pColorAttachments       = &colorAttachmentReference;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.pDepthStencilAttachment = nullptr;
    subpassDescription.pResolveAttachments     = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;

    // subpass dependency
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass          = 0;
    subpassDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask       = 0;
    subpassDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // render pass
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments    = &colorAttachment;
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies   = &subpassDependency;

    RAYCE_CHECK_VK(vkCreateRenderPass(mVkLogicalDeviceRef, &renderPassCreateInfo, nullptr, &mVkRenderPass), "Creating render pass failed!");
}

RenderPass::~RenderPass()
{
    if (mVkRenderPass)
    {
        vkDestroyRenderPass(mVkLogicalDeviceRef, mVkRenderPass, nullptr);
    }
}
