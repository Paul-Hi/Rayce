/// @file      imageMemoryBarrer.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMAGE_MEMORY_BARRIER_HPP
#define IMAGE_MEMORY_BARRIER_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{

    class RAYCE_API_EXPORT ImageMemoryBarrier
    {
      public:
        static void Create(const VkCommandBuffer commandBuffer, const VkImage image, const VkImageSubresourceRange subresourceRange, const VkAccessFlags srcAccessMask,
                           const VkAccessFlags dstAccessMask, const VkImageLayout oldLayout, const VkImageLayout newLayout)
        {
            VkImageMemoryBarrier barrier;
            barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext               = nullptr;
            barrier.srcAccessMask       = srcAccessMask;
            barrier.dstAccessMask       = dstAccessMask;
            barrier.oldLayout           = oldLayout;
            barrier.newLayout           = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image               = image;
            barrier.subresourceRange    = subresourceRange;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
    };

} // namespace rayce

#endif // IMAGE_MEMORY_BARRIER_HPP
