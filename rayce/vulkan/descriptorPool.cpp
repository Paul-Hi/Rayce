/// @file      descriptorPool.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/descriptorPool.hpp>
#include <vulkan/device.hpp>

using namespace rayce;

DescriptorPool::DescriptorPool(const std::unique_ptr<class Device>& logicalDevice, std::vector<VkDescriptorPoolSize> poolSizes, uint32 maxSets, VkDescriptorPoolCreateFlags flags)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext         = nullptr;
    descriptorPoolCreateInfo.flags         = flags | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolCreateInfo.maxSets       = maxSets;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes    = poolSizes.data();

    RAYCE_CHECK_VK(vkCreateDescriptorPool(mVkLogicalDeviceRef, &descriptorPoolCreateInfo, nullptr, &mVkDescriptorPool), "Creating descriptor pool failed!");

    RAYCE_LOG_INFO("Created descriptor pool!");
}

DescriptorPool::~DescriptorPool()
{
    if (mVkDescriptorPool)
    {
        vkDestroyDescriptorPool(mVkLogicalDeviceRef, mVkDescriptorPool, nullptr);
    }
}
