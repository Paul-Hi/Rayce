/// @file      image.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/buffer.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Image
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Image)

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

        void fillFromBuffer(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, const std::unique_ptr<Buffer>& buffer, VkExtent3D extent);

        template <class T>
        static void uploadImageDataWithStagingBuffer(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, Image& dstImage, const std::vector<T>& data, VkExtent3D extent)
        {
            Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, dstImage, data.data(), sizeof(data[0]) * data.size(), extent);
        }

        template <class T>
        static void uploadImageDataWithStagingBuffer(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, Image& dstImage, const T* data, uint32 size, VkExtent3D extent)
        {
            if (size == 0)
            {
                return;
            }

            std::unique_ptr<Buffer> stagingBuffer = std::make_unique<Buffer>(logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            stagingBuffer->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            const std::unique_ptr<DeviceMemory>& deviceMemory = stagingBuffer->getDeviceMemory();

            void* mapped = deviceMemory->map(0, size);
            std::memcpy(mapped, data, size);
            deviceMemory->unmap();

            dstImage.fillFromBuffer(logicalDevice, commandPool, stagingBuffer, extent);

            stagingBuffer.reset();
        }

        std::vector<byte> downloadImage(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool);

    private:
        VkDevice mVkLogicalDeviceRef;

        bool mOwned;
        VkExtent2D mExtent;
        VkFormat mFormat;
        VkImage mVkImage;
        VkImageLayout mVkImageLayout;

        std::unique_ptr<class DeviceMemory> pDeviceMemory;

        VkMemoryRequirements getMemoryRequirements() const;
    };
} // namespace rayce

#endif // IMAGE_HPP
