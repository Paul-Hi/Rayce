/// @file      deviceMemory.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef DEVICE_MEMORY_HPP
#define DEVICE_MEMORY_HPP

namespace rayce
{
    class RAYCE_API_EXPORT DeviceMemory
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(DeviceMemory)

        DeviceMemory(const std::unique_ptr<class Device>& logicalDevice, const ptr_size size, const uint32 memoryTypeBits, const VkMemoryAllocateFlags allocateFlags,
                     const VkMemoryPropertyFlags propertyFlags);
        ~DeviceMemory();

        VkDeviceMemory getVkDeviceMemory() const
        {
            return mVkDeviceMemory;
        }

        void* map(const ptr_size offset, const ptr_size size);
        void unmap();

    private:
        VkDeviceMemory mVkDeviceMemory;
        VkDevice mVkLogicalDeviceRef;

        uint32 findMemoryType(const uint32 typeFilter, const VkPhysicalDeviceMemoryProperties memoryProperties, const VkMemoryPropertyFlags propertyFlags);
    };
} // namespace rayce

#endif // DEVICE_MEMORY_HPP
