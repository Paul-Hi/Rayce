/// @file      swapchain.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/swapchain.hpp>

using namespace rayce;

Swapchain::Swapchain(const std::unique_ptr<Device>& logicalDevice, VkSurfaceKHR surface, GLFWwindow* nativeWindowHandle)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkPhysicalDevice physicalDevice = logicalDevice->getVkPhysicalDevice();
    // Capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    // Formats
    uint32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    // Present modes
    uint32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    RAYCE_CHECK(!formats.empty() && !presentModes.empty(), "No suitable swapchain found!");

    mFormat      = chooseSurfaceFormat(formats);
    mPresentMode = choosePresentMode(presentModes);
    mSwapExtent  = chooseSwapExtent(capabilities, nativeWindowHandle);

    // Image count, try at least more then minimum
    mMinImageCount    = capabilities.minImageCount + 1;
    uint32 imageCount = mMinImageCount;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = mFormat.format;
    createInfo.imageColorSpace  = mFormat.colorSpace;
    createInfo.imageExtent      = mSwapExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.preTransform     = capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = mPresentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = nullptr;

    if (logicalDevice->getGraphicsFamilyIndex() != logicalDevice->getPresentFamilyIndex())
    {
        uint32_t familyIndices[] = { logicalDevice->getGraphicsFamilyIndex(), logicalDevice->getPresentFamilyIndex() };

        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = familyIndices;
    }
    else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    VkDevice vkDevice = logicalDevice->getVkDevice();
    RAYCE_CHECK_VK(vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &mVkSwapchain), "Creating the swap chain failed!");

    vkGetSwapchainImagesKHR(vkDevice, mVkSwapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainImages(imageCount);
    vkGetSwapchainImagesKHR(vkDevice, mVkSwapchain, &imageCount, swapchainImages.data());

    createSwapchainImageViews(logicalDevice, swapchainImages);

    RAYCE_LOG_INFO("Created swap chain!");
}

Swapchain::~Swapchain()
{
    if (mVkSwapchain)
    {
        vkDestroySwapchainKHR(mVkLogicalDeviceRef, mVkSwapchain, nullptr);
    }
}

VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const VkSurfaceFormatKHR& candidate : formats)
    {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return candidate;
        }
    }

    RAYCE_ABORT("No suitable surface format available!");
}

VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    const VkPresentModeKHR requestedModes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
    const int32 requestedModesCount = 3;

    for (int32 requestedI = 0; requestedI < requestedModesCount; requestedI++)
        for (uint32 availableI = 0; availableI < presentModes.size(); availableI++)
            if (requestedModes[requestedI] == presentModes[availableI])
                return requestedModes[requestedI];

    return VK_PRESENT_MODE_FIFO_KHR; // Always available
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* nativeWindowHandle)
{
    // Return currentExtent, when it is set, since vulkan requires us to do this, else query window framebuffer size.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
    {
        return capabilities.currentExtent;
    }

    int32 width, height;
    glfwGetFramebufferSize(nativeWindowHandle, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32>(width), static_cast<uint32>(height) };

    actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void Swapchain::createSwapchainImageViews(const std::unique_ptr<Device>& logicalDevice, std::vector<VkImage>& swapchainImages)
{
    for (const VkImage& image : swapchainImages)
    {
        Image wrapped(logicalDevice, image);
        mSwapchainImageViews.push_back(std::make_unique<ImageView>(logicalDevice, wrapped, mFormat.format, VK_IMAGE_ASPECT_COLOR_BIT));
    }
}
