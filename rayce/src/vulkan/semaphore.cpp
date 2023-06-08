/// @file      semaphore.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/semaphore.hpp>

using namespace rayce;

Semaphore::Semaphore(const std::unique_ptr<class Device>& logicalDevice)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    RAYCE_CHECK_VK(vkCreateSemaphore(mVkLogicalDeviceRef, &semaphoreCreateInfo, nullptr, &mVkSemaphore), "Creating semaphore failed!");
}

Semaphore::~Semaphore()
{
    if (mVkSemaphore)
    {
        vkDestroySemaphore(mVkLogicalDeviceRef, mVkSemaphore, nullptr);
    }
}
