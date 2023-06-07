/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <rayceApp.hpp>
#include <vulkan/device.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/shaderModule.hpp>
#include <vulkan/surface.hpp>
#include <vulkan/swapchain.hpp>
#include <vulkan/window.hpp>
#include <vulkan/graphicsPipeline.hpp>

using namespace rayce;

RayceApp::RayceApp(const RayceOptions& options)
{
    mWindowWidth            = options.windowWidth;
    mWindowHeight           = options.windowHeight;
    mEnableValidationLayers = options.enableValidationLayers;

    pWindow = std::make_unique<Window>(mWindowWidth, mWindowHeight, options.name);

    std::vector<const char*> windowExtensions = pWindow->getVulkanExtensions();
    pInstance                                 = std::make_unique<Instance>(mEnableValidationLayers, windowExtensions, mValidationLayers);

    pSurface = std::make_unique<Surface>(pInstance->getVkInstance(), pWindow->getNativeWindowHandle());

    VkPhysicalDevice physicalDevice = pickPhysicalDevice();

    pDevice = std::make_unique<Device>(physicalDevice, pSurface->getVkSurface(), pInstance->getEnabledValidationLayers());

    pSwapchain = std::make_unique<Swapchain>(physicalDevice, pDevice, pSurface->getVkSurface(), pWindow->getNativeWindowHandle());

    pGraphicsPipeline = std::make_unique<GraphicsPipeline>(pDevice, pSwapchain, false);

    RAYCE_CHECK(onInitialize(), "onInitialize() failed!");

    RAYCE_LOG_INFO("Created RayceApp!");
}

RayceApp::~RayceApp()
{
    RAYCE_CHECK(onShutdown(), "onShutdown() failed!");
    RAYCE_LOG_INFO("Destroyed RayceApp!");
};

bool RayceApp::run()
{
    while (!pWindow->shouldClose())
    {
        pWindow->pollEvents();

        onUpdate();
        onRender();
        onImGuiRender();
    }
    return true;
}

bool RayceApp::onInitialize()
{
    return true;
}

bool RayceApp::onShutdown()
{
    return true;
}

void RayceApp::onUpdate() {}

void RayceApp::onRender() {}

void RayceApp::onImGuiRender() {}

VkPhysicalDevice RayceApp::pickPhysicalDevice()
{
    VkPhysicalDevice pickedDevice = nullptr;
    for (const VkPhysicalDevice& candidate : pInstance->getAvailableVkPhysicalDevices())
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(candidate, &deviceProperties);
        vkGetPhysicalDeviceFeatures(candidate, &deviceFeatures);

        if (!deviceFeatures.geometryShader)
        {
            continue;
        }

        uint32 extensionCount;
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, deviceExtensions.data());

        bool rayTracingAvailable = std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                                               [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0; });
        rayTracingAvailable &= std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                                           [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0; });

        if (!rayTracingAvailable)
        {
            continue;
        }

        const bool swapchainAvailable =
            std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });

        if (!swapchainAvailable)
        {
            continue;
        }

        uint32 queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyPropertyCount, queueFamilyProperties.data());

        const bool graphicsQueueFamilyAvailable =
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily) { return (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT); });

        if (!graphicsQueueFamilyAvailable)
        {
            continue;
        }

        const bool computeQueueFamilyAvailable =
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily) { return (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT); });

        if (!computeQueueFamilyAvailable)
        {
            continue;
        }

        const bool presentQueueFamilyAvailable = std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                             [&](const VkQueueFamilyProperties& queueFamily)
                                                             {
                                                                 VkBool32 presentSupport = false;
                                                                 const uint32 i          = static_cast<uint32>(&*queueFamilyProperties.cbegin() - &queueFamily);
                                                                 vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, pSurface->getVkSurface(), &presentSupport);
                                                                 return queueFamily.queueCount > 0 && presentSupport;
                                                             });

        if (!presentQueueFamilyAvailable)
        {
            continue;
        }

        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            RAYCE_LOG_WARN("Device is not a discrete GPU. This could result in really bad performance!");
        }

        pickedDevice = candidate;
        RAYCE_LOG_INFO("Using device: %s!", deviceProperties.deviceName);
        break;
    }

    RAYCE_CHECK_NOTNULL(pickedDevice, "No compatible physical device available!");

    return pickedDevice;
}
