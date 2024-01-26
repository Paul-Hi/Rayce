/// @file      semaphore.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

namespace rayce
{
    // binary
    class RAYCE_API_EXPORT Semaphore
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Semaphore)

        Semaphore(const std::unique_ptr<class Device>& logicalDevice);
        ~Semaphore();

        VkSemaphore getVkSemaphore() const
        {
            return mVkSemaphore;
        }

    private:
        VkSemaphore mVkSemaphore;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // SEMAPHORE_HPP
