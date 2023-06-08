/// @file      semaphore.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    // binary
    class RAYCE_API_EXPORT Semaphore
    {
      public:
        DISABLE_COPY_MOVE_VK(Semaphore)

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
