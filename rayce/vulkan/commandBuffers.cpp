/// @file      commandBuffers.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>

using namespace rayce;

CommandBuffers::CommandBuffers(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const uint32 number)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mVkCommandPoolRef(commandPool->getVkCommandPool())
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = mVkCommandPoolRef;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = number;

    mVkCommandBuffers.resize(number);

    RAYCE_CHECK_VK(vkAllocateCommandBuffers(mVkLogicalDeviceRef, &allocInfo, mVkCommandBuffers.data()), "Allocating command buffers failed!");
}

CommandBuffers::~CommandBuffers()
{
    if (!mVkCommandBuffers.empty())
    {
        vkFreeCommandBuffers(mVkLogicalDeviceRef, mVkCommandPoolRef, static_cast<uint32>(mVkCommandBuffers.size()), mVkCommandBuffers.data());
        mVkCommandBuffers.clear();
    }
}

VkCommandBuffer CommandBuffers::beginCommandBuffer(uint32 idx)
{
    uint32 available = static_cast<uint32>(mVkCommandBuffers.size());

    RAYCE_CHECK_LT(idx, available, "Begin: Index %d out of bounds. Only %d command buffers available!", idx, available);

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    // Not sure if the reset has to happen with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    // RAYCE_CHECK_VK(vkResetCommandBuffer(mVkCommandBuffers[idx], 0), "Reset command buffer failed!");
    RAYCE_CHECK_VK(vkBeginCommandBuffer(mVkCommandBuffers[idx], &commandBufferBeginInfo), "Beginning command buffer failed!");

    return mVkCommandBuffers[idx];
}

void CommandBuffers::endCommandBuffer(uint32 idx)
{
    uint32 available = static_cast<uint32>(mVkCommandBuffers.size());

    RAYCE_CHECK_LT(idx, available, "End: Index %d out of bounds. Only %d command buffers available!", idx, available);

    RAYCE_CHECK_VK(vkEndCommandBuffer(mVkCommandBuffers[idx]), "Ending command buffer failed!");
}
