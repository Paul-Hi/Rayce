/// @file      image.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/deviceMemory.hpp>
#include <vulkan/image.hpp>
#include <vulkan/immediateSubmit.hpp>

using namespace rayce;

Image::Image(const std::unique_ptr<class Device>& logicalDevice, VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mOwned(true)
    , mVkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent        = { extent.width, extent.height, 1 };
    imageCreateInfo.mipLevels     = 1;
    imageCreateInfo.arrayLayers   = 1;
    imageCreateInfo.format        = format;
    imageCreateInfo.tiling        = tiling;
    imageCreateInfo.initialLayout = mVkImageLayout;
    imageCreateInfo.usage         = usage;
    imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags         = 0;

    RAYCE_CHECK_VK(vkCreateImage(mVkLogicalDeviceRef, &imageCreateInfo, nullptr, &mVkImage), "Creating image failed!");
}

Image::Image(const std::unique_ptr<class Device>& logicalDevice, VkImage image)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mOwned(false)
    , mVkImage(image)
    , mVkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

void Image::allocateMemory(const std::unique_ptr<Device>& logicalDevice, const VkMemoryAllocateFlags allocateFlags, const VkMemoryPropertyFlags propertyFlags)
{
    const VkMemoryRequirements requirements = getMemoryRequirements();
    pDeviceMemory.reset(new DeviceMemory(logicalDevice, requirements.size, requirements.memoryTypeBits, allocateFlags, propertyFlags));

    RAYCE_CHECK_VK(vkBindImageMemory(mVkLogicalDeviceRef, mVkImage, pDeviceMemory->getVkDeviceMemory(), 0), "Binding image device memory failed!");
}

void Image::adaptImageLayout(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, VkImageLayout newLayout)
{
    if (!mOwned)
    {
        RAYCE_LOG_WARN("Can not adapt image layout of image which is not owned!");
        return;
    }

    ImmediateSubmit::Execute(logicalDevice, commandPool,
                             [&](VkCommandBuffer commandBuffer)
                             {
                                 VkImageMemoryBarrier barrier            = {};
                                 barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                 barrier.oldLayout                       = mVkImageLayout;
                                 barrier.newLayout                       = newLayout;
                                 barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                                 barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                                 barrier.image                           = mVkImage;
                                 barrier.subresourceRange.baseMipLevel   = 0;
                                 barrier.subresourceRange.levelCount     = 1;
                                 barrier.subresourceRange.baseArrayLayer = 0;
                                 barrier.subresourceRange.layerCount     = 1;

                                 if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                                 {
                                     // not possible atm
                                     // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                                     // if (hasStencil)
                                     // {
                                     //     barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                                     // }
                                 }
                                 else
                                 {
                                     barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                 }

                                 VkPipelineStageFlags sourceStage;
                                 VkPipelineStageFlags destinationStage;

                                 if (mVkImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                                 {
                                     barrier.srcAccessMask = 0;
                                     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                                     sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                     destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                 }
                                 else if (mVkImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                 {
                                     barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                     barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                                     sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                     destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                 }
                                 else if (mVkImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                                 {
                                     barrier.srcAccessMask = 0;
                                     barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                                     sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                     destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                                 }
                                 else
                                 {
                                     RAYCE_ABORT("Layout adaption not supported!");
                                 }

                                 vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
                             });

    mVkImageLayout = newLayout;
}

Image::~Image()
{
    if (mOwned && mVkImage)
    {
        vkDestroyImage(mVkLogicalDeviceRef, mVkImage, nullptr);
    }
}

VkMemoryRequirements Image::getMemoryRequirements() const
{
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mVkLogicalDeviceRef, mVkImage, &memoryRequirements);

    return memoryRequirements;
}