/// @file      framebuffer.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

namespace rayce
{
    class RAYCE_API_EXPORT Framebuffer
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Framebuffer)

        Framebuffer(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const std::unique_ptr<class RenderPass>& renderPass,
                    const std::unique_ptr<class ImageView>& imageView);
        ~Framebuffer();

        VkFramebuffer getVkFramebuffer() const
        {
            return mVkFramebuffer;
        }

    private:
        VkFramebuffer mVkFramebuffer;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // FRAMEBUFFER_HPP
