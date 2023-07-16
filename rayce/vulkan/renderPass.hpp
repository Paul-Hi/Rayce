/// @file      renderPass.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RENDER_PASS_HPP
#define RENDER_PASS_HPP

namespace rayce
{
    class RAYCE_API_EXPORT RenderPass
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(RenderPass)

        RenderPass(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const VkAttachmentLoadOp colorBufferLoadOp);
        ~RenderPass();

        VkRenderPass getVkRenderPass() const
        {
            return mVkRenderPass;
        }

    private:
        VkRenderPass mVkRenderPass;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // RENDER_PASS_HPP
