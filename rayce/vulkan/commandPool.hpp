/// @file      commandPool.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef COMMAND_POOL_HPP
#define COMMAND_POOL_HPP

namespace rayce
{
    class RAYCE_API_EXPORT CommandPool
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(CommandPool)

        CommandPool(const std::unique_ptr<class Device>& logicalDevice, VkCommandPoolCreateFlags flags);
        ~CommandPool();

        VkCommandPool getVkCommandPool() const
        {
            return mVkCommandPool;
        }

    private:
        VkCommandPool mVkCommandPool;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // COMMAND_POOL_HPP
