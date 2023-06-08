/// @file      imageView.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMAGE_VIEW_HPP
#define IMAGE_VIEW_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT ImageView
    {
      public:
        DISABLE_COPY_MOVE_VK(ImageView)

        ImageView(const std::unique_ptr<class Device>& logicalDevice, VkImage image, VkFormat format, VkImageAspectFlagBits aspectMask);
        ~ImageView();

        VkImageView getVkImageView() const
        {
            return mVkImageView;
        }

      private:
        VkImageView mVkImageView;
        VkImage mVkBaseImageRef;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // IMAGE_VIEW_HPP
