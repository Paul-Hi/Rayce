/// @file      imageView.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/imageView.hpp>

using namespace rayce;

ImageView::ImageView(const std::unique_ptr<Device>& logicalDevice, VkImage image, VkFormat format, VkImageAspectFlagBits aspectMask)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mVkBaseImageRef(image)
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask     = aspectMask;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    RAYCE_CHECK_VK(vkCreateImageView(mVkLogicalDeviceRef, &createInfo, nullptr, &mVkImageView), "Creating image view failed!");
}

ImageView::~ImageView()
{
    if (mVkImageView)
    {
        vkDestroyImageView(mVkLogicalDeviceRef, mVkImageView, nullptr);
    }
}
