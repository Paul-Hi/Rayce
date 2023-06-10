/// @file      immediateSubmit.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMMEDIATE_SUBMIT_HPP
#define IMMEDIATE_SUBMIT_HPP

#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>

namespace rayce
{
    class ImmediateSubmit
    {
      public:
        static void Execute(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const std::function<void(VkCommandBuffer)>& immediateFunction)
        {
            CommandBuffers commandBuffers(logicalDevice, commandPool, 1);

            VkCommandBufferBeginInfo commandBufferBeginInfo = {};
            commandBufferBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffers[0], &commandBufferBeginInfo);

            immediateFunction(commandBuffers[0]);

            vkEndCommandBuffer(commandBuffers[0]);

            VkSubmitInfo submitInfo       = {};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &commandBuffers[0];

            VkQueue graphicsQueue = logicalDevice->getVkGraphicsQueue();
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
            vkQueueWaitIdle(graphicsQueue);
        }
    };

} // namespace rayce

#endif // IMMEDIATE_SUBMIT_HPP
