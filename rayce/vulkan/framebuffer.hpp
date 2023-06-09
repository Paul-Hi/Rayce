/// @file      framebuffer.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

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
