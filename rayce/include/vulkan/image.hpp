/// @file      image.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Image
    {
      public:
        DISABLE_COPY_MOVE_VK(Image)

        Image(const std::unique_ptr<class Device>& logicalDevice, VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);
        Image(const std::unique_ptr<class Device>& logicalDevice, VkImage image);
        ~Image();

        VkImage getVkImage() const
        {
            return mVkImage;
        }

        const std::unique_ptr<class DeviceMemory>& getDeviceMemory() const
        {
            return pDeviceMemory;
        }

        void adaptImageLayout(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, VkImageLayout newLayout);

        void allocateMemory(const std::unique_ptr<class Device>& logicalDevice, const VkMemoryAllocateFlags allocateFlags, const VkMemoryPropertyFlags propertyFlags);

      private:
        VkDevice mVkLogicalDeviceRef;

        bool mOwned;
        VkImage mVkImage;
        VkImageLayout mVkImageLayout;

        std::unique_ptr<class DeviceMemory> pDeviceMemory;

        VkMemoryRequirements getMemoryRequirements() const;
    };
} // namespace rayce

#endif // IMAGE_HPP
