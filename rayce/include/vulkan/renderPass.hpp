/// @file      renderPass.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef RENDER_PASS_HPP
#define RENDER_PASS_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT RenderPass
    {
      public:
        DISABLE_COPY_MOVE_VK(RenderPass)

        RenderPass(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain);
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
