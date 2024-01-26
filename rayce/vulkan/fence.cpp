/// @file      fence.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/fence.hpp>

using namespace rayce;

Fence::Fence(const std::unique_ptr<class Device>& logicalDevice, bool createSignaled)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    RAYCE_CHECK_VK(vkCreateFence(mVkLogicalDeviceRef, &fenceCreateInfo, nullptr, &mVkFence), "Creating fence failed!");
}

void Fence::reset()
{
    RAYCE_CHECK_VK(vkResetFences(mVkLogicalDeviceRef, 1, &mVkFence), "Reset fence failed!");
}

void Fence::wait(const uint64 timeout)
{
    RAYCE_CHECK_VK(vkWaitForFences(mVkLogicalDeviceRef, 1, &mVkFence, VK_TRUE, timeout), "Waiting for fence failed!");
}

Fence::~Fence()
{
    if (mVkFence)
    {
        vkDestroyFence(mVkLogicalDeviceRef, mVkFence, nullptr);
    }
}
