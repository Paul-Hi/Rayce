/// @file      imageView.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>

using namespace rayce;

ImageView::ImageView(const std::unique_ptr<Device>& logicalDevice, Image& image, VkFormat format, VkImageAspectFlagBits aspectMask)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mVkBaseImageRef(image.getVkImage())
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = mVkBaseImageRef;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask     = aspectMask;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

    RAYCE_CHECK_VK(vkCreateImageView(mVkLogicalDeviceRef, &createInfo, nullptr, &mVkImageView), "Creating image view failed!");
}

ImageView::~ImageView()
{
    if (mVkImageView)
    {
        vkDestroyImageView(mVkLogicalDeviceRef, mVkImageView, nullptr);
    }
}
