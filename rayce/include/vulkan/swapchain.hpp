/// @file      swapchain.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Swapchain
    {
      public:
        DISABLE_COPY_MOVE_VK(Swapchain)

        Swapchain(VkPhysicalDevice physicalDevice, const std::unique_ptr<class Device>& logicalDevice, VkSurfaceKHR surface, GLFWwindow* nativeWindowHandle);
        ~Swapchain();

        VkSwapchainKHR getVkSwapchain()
        {
            return mVkSwapchain;
        }

      private:
        VkSwapchainKHR mVkSwapchain;
        VkDevice mVkLogicalDeviceRef;

        std::vector<VkImage> mSwapchainImages;
        std::vector<VkImageView> mSwapchainImageViews;

        uint32 mMinImageCount;
        VkPresentModeKHR mPresentMode;
        VkExtent2D mSwapExtent;
        VkSurfaceFormatKHR mFormat;

        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* nativeWindowHandle);
        void createSwapchainImageViews();
    };
} // namespace rayce

#endif // SWAPCHAIN_HPP
