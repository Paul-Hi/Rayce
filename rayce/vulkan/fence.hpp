/// @file      fence.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef FENCE_HPP
#define FENCE_HPP

namespace rayce
{
    /// @brief A binary fence to synchronize GPU and CPU.
    class RAYCE_API_EXPORT Fence
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Fence)

        /// @brief Constructs a new @a Fence.
        /// @param[in] logicalDevice The logical @a Device.
        /// @param[in] createSignaled Create the @a Fence signaled (saves one call).
        Fence(const std::unique_ptr<class Device>& logicalDevice, bool createSignaled);

        /// @brief Destructor.
        ~Fence();

        /// @brief Retrieves the vulkan fence handle.
        /// @return The vulkan fence handle.
        VkFence getVkFence() const
        {
            return mVkFence;
        }

        /// @brief Resets the @a Fence.
        void reset();

        /// @brief Waits for the @a Fence to get signaled.
        /// @param[in] timeout Maximum waiting time.
        void wait(const uint64 timeout);

    private:
        /// @brief The vulkan fence handle.
        VkFence mVkFence;
        /// @brief The vulkan handle referencing the vulkan device.
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // FENCE_HPP
