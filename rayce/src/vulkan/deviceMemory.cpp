/// @file      deviceMemory.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/deviceMemory.hpp>

using namespace rayce;

DeviceMemory::DeviceMemory(const std::unique_ptr<Device>& logicalDevice, const ptr_size size, const uint32 memoryTypeBits, const VkMemoryAllocateFlags allocateFLags,
                           const VkMemoryPropertyFlags propertyFlags)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(logicalDevice->getVkPhysicalDevice(), &memoryProperties);

    uint32 typeIdx = findMemoryType(memoryTypeBits, memoryProperties, propertyFlags);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize  = size;
    allocateInfo.memoryTypeIndex = typeIdx;

    RAYCE_CHECK_VK(vkAllocateMemory(mVkLogicalDeviceRef, &allocateInfo, nullptr, &mVkDeviceMemory), "Allocating device memory failed!");
}

DeviceMemory::~DeviceMemory()
{
    if (mVkDeviceMemory)
    {
        vkFreeMemory(mVkLogicalDeviceRef, mVkDeviceMemory, nullptr);
    }
}

void* DeviceMemory::map(const ptr_size offset, const ptr_size size)
{
    void* data;
    RAYCE_CHECK_VK(vkMapMemory(mVkLogicalDeviceRef, mVkDeviceMemory, offset, size, 0, &data), "Mapping device memory failed!");

    return data;
}

void DeviceMemory::unmap()
{
    vkUnmapMemory(mVkLogicalDeviceRef, mVkDeviceMemory);
}

uint32 DeviceMemory::findMemoryType(const uint32 typeFilter, const VkPhysicalDeviceMemoryProperties memoryProperties, const VkMemoryPropertyFlags propertyFlags)
{
    for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            return i;
        }
    }

    RAYCE_ABORT("No suitable memory format available!");
    return -1;
}
