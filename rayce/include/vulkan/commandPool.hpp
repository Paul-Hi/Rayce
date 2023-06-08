/// @file      commandPool.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef COMMAND_POOL_HPP
#define COMMAND_POOL_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT CommandPool
    {
      public:
        DISABLE_COPY_MOVE_VK(CommandPool)

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
