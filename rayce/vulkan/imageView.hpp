/// @file      imageView.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMAGE_VIEW_HPP
#define IMAGE_VIEW_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

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
