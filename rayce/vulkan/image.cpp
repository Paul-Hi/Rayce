/// @file      image.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/deviceMemory.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageMemoryBarrier.hpp>
#include <vulkan/immediateSubmit.hpp>

using namespace rayce;

Image::Image(const std::unique_ptr<class Device>& logicalDevice, VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mOwned(true)
    , mExtent(extent)
    , mFormat(format)
    , mVkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent        = { mExtent.width, mExtent.height, 1 };
    imageCreateInfo.mipLevels     = 1;
    imageCreateInfo.arrayLayers   = 1;
    imageCreateInfo.format        = mFormat;
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
                                 barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                                 barrier.subresourceRange.baseArrayLayer = 0;
                                 barrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

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
                                 else if (mVkImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) // FIXME: We should unify thet with image memory barrier?
                                 {
                                     barrier.srcAccessMask = 0;
                                     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                                     sourceStage      = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                                     destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
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

void Image::fillFromBuffer(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const std::unique_ptr<Buffer>& buffer, VkExtent3D extent)
{
    ImmediateSubmit::Execute(logicalDevice, commandPool,
                             [&](VkCommandBuffer commandBuffer)
                             {
                                 VkBufferImageCopy copyRegion = {};
                                 copyRegion.bufferOffset      = 0;
                                 copyRegion.bufferRowLength   = 0;
                                 copyRegion.bufferImageHeight = 0;

                                 copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                                 copyRegion.imageSubresource.mipLevel       = 0;
                                 copyRegion.imageSubresource.baseArrayLayer = 0;
                                 copyRegion.imageSubresource.layerCount     = 1;

                                 copyRegion.imageOffset = { 0, 0, 0 };
                                 copyRegion.imageExtent = extent;

                                 vkCmdCopyBufferToImage(commandBuffer, buffer->getVkBuffer(), mVkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
                             });
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

std::vector<byte> Image::downloadImage(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool)
{
    Image tmpImage(logicalDevice, mExtent, mFormat, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    tmpImage.allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkImage srcImage = getVkImage();
    VkImage dstImage = tmpImage.getVkImage();

    ImmediateSubmit::Execute(logicalDevice, commandPool,
                             [&](VkCommandBuffer commandBuffer)
                             {
                                 VkImageSubresourceRange subresourceRange{};
                                 subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                                 subresourceRange.baseMipLevel   = 0;
                                 subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                                 subresourceRange.baseArrayLayer = 0;
                                 subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

                                 ImageMemoryBarrier::Create(commandBuffer, srcImage, subresourceRange, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                                 ImageMemoryBarrier::Create(commandBuffer, dstImage, subresourceRange, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                                 // FIXME: Copy kills the device...
                                 VkImageCopy copyRegion{};
                                 copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                                 copyRegion.srcOffset      = { 0, 0, 0 };
                                 copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                                 copyRegion.dstOffset      = { 0, 0, 0 };
                                 copyRegion.extent         = { mExtent.width, mExtent.height, 1 };

                                 vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                                 ImageMemoryBarrier::Create(commandBuffer, srcImage, subresourceRange, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
                                 ImageMemoryBarrier::Create(commandBuffer, dstImage, subresourceRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
                             });

    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(logicalDevice->getVkDevice(), dstImage, &subResource, &subResourceLayout);

    const byte* tmpData;
    vkMapMemory(logicalDevice->getVkDevice(), tmpImage.getDeviceMemory()->getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, (void**)&tmpData);
    tmpData += subResourceLayout.offset;

    std::vector<byte> result;
    result.reserve(subResourceLayout.size);

    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    const bool colorSwizzle          = (std::find(formatsBGR.begin(), formatsBGR.end(), mFormat) != formatsBGR.end());

    for (int32 y = 0; y < mExtent.height; ++y)
    {
        uint32* row = (uint32*)tmpData;
        for (int32 x = 0; x < mExtent.width; ++x)
        {
            if (colorSwizzle)
            {
                result.push_back(*((byte*)row + 2));
                result.push_back(*((byte*)row + 1));
                result.push_back(*((byte*)row));
                result.push_back(*((byte*)row + 3));
            }
            else
            {
                result.push_back(*((byte*)row));
                result.push_back(*((byte*)row + 1));
                result.push_back(*((byte*)row + 2));
                result.push_back(*((byte*)row + 3));
            }
            row++;
        }
        tmpData += subResourceLayout.rowPitch;
    }

    vkUnmapMemory(logicalDevice->getVkDevice(), tmpImage.getDeviceMemory()->getVkDeviceMemory());

    return result;
}
