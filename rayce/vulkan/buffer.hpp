/// @file      buffer.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef BUFFER_HPP
#define BUFFER_HPP
#include <vulkan/commandPool.hpp>
#include <vulkan/deviceMemory.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Buffer
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Buffer)

        Buffer(const std::unique_ptr<class Device>& logicalDevice, const ptr_size size, const VkBufferUsageFlags usage);
        ~Buffer();

        VkBuffer getVkBuffer() const
        {
            return mVkBuffer;
        }

        const std::unique_ptr<DeviceMemory>& getDeviceMemory() const
        {
            return pDeviceMemory;
        }

        VkDeviceAddress getDeviceAddress() const;

        void allocateMemory(const std::unique_ptr<class Device>& logicalDevice, const VkMemoryAllocateFlags allocateFlags, const VkMemoryPropertyFlags propertyFlags);

        void fillFrom(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const Buffer& src, VkDeviceSize size);

        template <class T>
        static void uploadDataWithStagingBuffer(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, Buffer& dstBuffer, const std::vector<T>& data)
        {
            const ptr_size size = sizeof(data[0]) * data.size();

            if (size == 0)
            {
                return;
            }

            std::unique_ptr<Buffer> stagingBuffer = std::make_unique<Buffer>(logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            stagingBuffer->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            const std::unique_ptr<DeviceMemory>& deviceMemory = stagingBuffer->getDeviceMemory();

            void* mapped = deviceMemory->map(0, size);
            std::memcpy(mapped, data.data(), size);
            deviceMemory->unmap();

            dstBuffer.fillFrom(logicalDevice, commandPool, *stagingBuffer, size);

            stagingBuffer.reset();
        }

    private:
        VkBuffer mVkBuffer;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<DeviceMemory> pDeviceMemory;

        VkMemoryRequirements getMemoryRequirements() const;
    };
} // namespace rayce

#endif // BUFFER_HPP
