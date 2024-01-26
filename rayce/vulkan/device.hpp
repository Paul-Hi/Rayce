/// @file      device.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef DEVICE_HPP
#define DEVICE_HPP

namespace slang
{
    struct IGlobalSession;
};
namespace rayce
{
    class RAYCE_API_EXPORT Device
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Device)

        Device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& enabledValidationLayers, bool raytracingSupported);
        ~Device();

        VkDevice getVkDevice() const
        {
            return mVkDevice;
        }

        VkQueue getVkGraphicsQueue() const
        {
            return mVkGraphicsQueue;
        }

        VkQueue getVkComputeQueue() const
        {
            return mVkComputeQueue;
        }

        VkQueue getVkPresentQueue() const
        {
            return mVkPresentQueue;
        }

        VkPhysicalDevice getVkPhysicalDevice() const
        {
            return mVkPhysicalDevice;
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

        VkPhysicalDeviceProperties getProperties() const
        {
            return mProperties;
        }

        slang::IGlobalSession* getSlangGlobalSession() const
        {
            return mSlangGlobalSession;
        }

    private:
        VkDevice mVkDevice;
        VkQueue mVkGraphicsQueue;
        VkQueue mVkComputeQueue;
        VkQueue mVkPresentQueue;
        VkPhysicalDevice mVkPhysicalDevice;

        uint32 mGraphicsFamilyIndex;
        uint32 mComputeFamilyIndex;
        uint32 mPresentFamilyIndex;

        VkPhysicalDeviceProperties mProperties;

        VkDeviceQueueCreateInfo createVkQueue(uint32 queueFamilyIndex);

        // Slang GlobalSession
        slang::IGlobalSession* mSlangGlobalSession;
    };
} // namespace rayce

#endif // DEVICE_HPP
