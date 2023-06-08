/// @file      commandBuffers.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef COMMAND_BUFFERS_HPP
#define COMMAND_BUFFERS_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT CommandBuffers
    {
      public:
        DISABLE_COPY_MOVE_VK(CommandBuffers)

        CommandBuffers(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, const uint32 number);
        ~CommandBuffers();

        VkCommandBuffer beginCommandBuffer(const uint32 idx);
        void endCommandBuffer(const uint32 idx);

      private:
        std::vector<VkCommandBuffer> mVkCommandBuffers;
        VkDevice mVkLogicalDeviceRef;
        VkCommandPool mVkCommandPoolRef;
    };
} // namespace rayce

#endif // COMMAND_BUFFERS_HPP
