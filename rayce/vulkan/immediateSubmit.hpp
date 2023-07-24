/// @file      immediateSubmit.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef IMMEDIATE_SUBMIT_HPP
#define IMMEDIATE_SUBMIT_HPP

#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>

namespace rayce
{
    /// @brief Class to immediatly execute any vulkan command.
    /// @details Creates a new @a CommandBuffer , inserts commands and flushes it directly.
    class ImmediateSubmit
    {
    public:
        /// @brief Executes any function including vulkan commands immediately.
        /// @param[in] logicalDevice The logical vulkan @a Device.
        /// @param[in] commandPool The @a CommandPool to get the buffer from.
        /// @param[in] immediateFunction A function pointer to the commands to execute immediately.
        static void Execute(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const std::function<void(VkCommandBuffer)>& immediateFunction)
        {
            CommandBuffers commandBuffers(logicalDevice, commandPool, 1);

            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            RAYCE_CHECK_VK(vkResetCommandBuffer(commandBuffers[0], 0), "vkResetCommandBuffer");
            RAYCE_CHECK_VK(vkBeginCommandBuffer(commandBuffers[0], &commandBufferBeginInfo), "vkBeginCommandBuffer");

            immediateFunction(commandBuffers[0]);

            RAYCE_CHECK_VK(vkEndCommandBuffer(commandBuffers[0]), "vkEndCommandBuffer");

            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &commandBuffers[0];

            VkQueue graphicsQueue = logicalDevice->getVkGraphicsQueue();
            RAYCE_CHECK_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit");
            RAYCE_CHECK_VK(vkQueueWaitIdle(graphicsQueue), "vkQueueWaitIdle");
        }
    };

} // namespace rayce

#endif // IMMEDIATE_SUBMIT_HPP
