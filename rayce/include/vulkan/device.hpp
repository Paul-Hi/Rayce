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
        DISABLE_COPY_MOVE_VK(Device)

        Device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& enabledValidationLayers);
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

        VkQueue getVkPresentQueue()
        {
            return mVkPresentQueue;
        }

        uint32 getGraphicsFamilyIndex() const
        {
            return mGraphicsFamilyIndex;
        }

        uint32 getComputeFamilyIndex() const
        {
            return mComputeFamilyIndex;
        }

        uint32 getPresentFamilyIndex() const
        {
            return mPresentFamilyIndex;
        }

      private:
        VkDevice mVkDevice;
        VkQueue mVkGraphicsQueue;
        VkQueue mVkComputeQueue;
        VkQueue mVkPresentQueue;

        uint32 mGraphicsFamilyIndex;
        uint32 mComputeFamilyIndex;
        uint32 mPresentFamilyIndex;

        VkDeviceQueueCreateInfo createVkQueue(uint32 queueFamilyIndex);
    };
} // namespace rayce

#endif // DEVICE_HPP
