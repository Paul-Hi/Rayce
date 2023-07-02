/// @file      commandPool.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vk/commandPool.hpp>
#include <vk/device.hpp>

using namespace rayce;

CommandPool::CommandPool(const std::unique_ptr<class Device>& logicalDevice, VkCommandPoolCreateFlags flags)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags            = flags;
    commandPoolCreateInfo.queueFamilyIndex = logicalDevice->getGraphicsFamilyIndex();

    RAYCE_CHECK_VK(vkCreateCommandPool(mVkLogicalDeviceRef, &commandPoolCreateInfo, nullptr, &mVkCommandPool), "Creating command pool failed!");

    RAYCE_LOG_INFO("Created command pool!");
}

CommandPool::~CommandPool()
{
    if (mVkCommandPool)
    {
        vkDestroyCommandPool(mVkLogicalDeviceRef, mVkCommandPool, nullptr);
    }
}
