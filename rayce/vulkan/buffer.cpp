/// @file      buffer.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/buffer.hpp>
#include <vulkan/device.hpp>
#include <vulkan/immediateSubmit.hpp>
#include <vulkan/vertex.hpp>

using namespace rayce;

Buffer::Buffer(const std::unique_ptr<Device>& logicalDevice, const ptr_size size, const VkBufferUsageFlags usage)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size        = size;
    bufferCreateInfo.usage       = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.flags       = NULL;

    RAYCE_CHECK_VK(vkCreateBuffer(mVkLogicalDeviceRef, &bufferCreateInfo, nullptr, &mVkBuffer), "Creating buffer failed!");
}

Buffer::~Buffer()
{
    if (mVkBuffer)
    {
        vkDestroyBuffer(mVkLogicalDeviceRef, mVkBuffer, nullptr);
    }
}

void Buffer::allocateMemory(const std::unique_ptr<Device>& logicalDevice, const VkMemoryAllocateFlags allocateFlags, const VkMemoryPropertyFlags propertyFlags)
{
    const VkMemoryRequirements requirements = getMemoryRequirements();
    pDeviceMemory.reset(new DeviceMemory(logicalDevice, requirements.size, requirements.memoryTypeBits, allocateFlags, propertyFlags));

    RAYCE_CHECK_VK(vkBindBufferMemory(mVkLogicalDeviceRef, mVkBuffer, pDeviceMemory->getVkDeviceMemory(), 0), "Binding buffer device memory failed!");
}

VkDeviceAddress Buffer::getDeviceAddress() const
{
    VkBufferDeviceAddressInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.pNext  = nullptr;
    info.buffer = mVkBuffer;

    return vkGetBufferDeviceAddress(mVkLogicalDeviceRef, &info);
}

void Buffer::fillFrom(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const Buffer& srcBuffer, VkDeviceSize size)
{
    ImmediateSubmit::Execute(logicalDevice, commandPool,
                             [&](VkCommandBuffer commandBuffer)
                             {
                                 VkBufferCopy copyRegion = {};
                                 copyRegion.srcOffset    = 0;
                                 copyRegion.dstOffset    = 0;
                                 copyRegion.size         = size;

                                 vkCmdCopyBuffer(commandBuffer, srcBuffer.getVkBuffer(), mVkBuffer, 1, &copyRegion);
                             });
}

VkMemoryRequirements Buffer::getMemoryRequirements() const
{
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mVkLogicalDeviceRef, mVkBuffer, &memoryRequirements);

    return memoryRequirements;
}
