/// @file      fence.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef FENCE_HPP
#define FENCE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    // binary
    class RAYCE_API_EXPORT Fence
    {
      public:
        DISABLE_COPY_MOVE_VK(Fence)

        Fence(const std::unique_ptr<class Device>& logicalDevice, bool createSignaled);
        ~Fence();

        VkFence getVkFence() const
        {
            return mVkFence;
        }

        void reset();
        void wait(const uint64 timeout);

      private:
        VkFence mVkFence;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // FENCE_HPP
