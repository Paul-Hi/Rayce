/// @file      imageView.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef IMAGE_VIEW_HPP
#define IMAGE_VIEW_HPP

namespace rayce
{
    class RAYCE_API_EXPORT ImageView
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(ImageView)

        ImageView(const std::unique_ptr<class Device>& logicalDevice, class Image& image, VkFormat format, VkImageAspectFlagBits aspectMask);
        ~ImageView();

        VkImageView getVkImageView() const
        {
            return mVkImageView;
        }

        VkImage getVkImage() const
        {
            return mVkBaseImageRef;
        }

    private:
        VkImageView mVkImageView;
        VkDevice mVkLogicalDeviceRef;
        VkImage mVkBaseImageRef;
    };
} // namespace rayce

#endif // IMAGE_VIEW_HPP
