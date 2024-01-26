/// @file      framebuffer.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/renderPass.hpp>
#include <vulkan/swapchain.hpp>

using namespace rayce;

Framebuffer::Framebuffer(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<Swapchain>& swapchain, const std::unique_ptr<RenderPass>& renderPass,
                         const std::unique_ptr<ImageView>& imageView)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkImageView attachments[] = { imageView->getVkImageView() };

    VkExtent2D swapExtent = swapchain->getSwapExtent();
    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass      = renderPass->getVkRenderPass();
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.pAttachments    = attachments;
    framebufferCreateInfo.width           = swapExtent.width;
    framebufferCreateInfo.height          = swapExtent.height;
    framebufferCreateInfo.layers          = 1;

    RAYCE_CHECK_VK(vkCreateFramebuffer(mVkLogicalDeviceRef, &framebufferCreateInfo, nullptr, &mVkFramebuffer), "Creating framebuffer failed!");
}

Framebuffer::~Framebuffer()
{
    if (mVkFramebuffer)
    {
        vkDestroyFramebuffer(mVkLogicalDeviceRef, mVkFramebuffer, nullptr);
    }
}
