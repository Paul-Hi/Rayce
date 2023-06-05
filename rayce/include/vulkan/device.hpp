/// @file      device.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Device
    {
      public:
        Device(VkPhysicalDevice physicalDevice, const std::vector<const char*>& enabledValidationLayers);
        ~Device();

        VkDevice getVkDevice()
        {
            return mVkDevice;
        }

        VkQueue getVkGraphicsQueue()
        {
            return mVkGraphicsQueue;
        }

        VkQueue getVkComputeQueue()
        {
            return mVkComputeQueue;
        }

      private:
        VkDevice mVkDevice;
        VkQueue mVkGraphicsQueue;
        VkQueue mVkComputeQueue;

        VkDeviceQueueCreateInfo createVkQueue(uint32 queueFamilyIndex);
    };
} // namespace rayce

#endif // DEVICE_HPP
