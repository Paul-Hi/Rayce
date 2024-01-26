/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <app/imguiInterface.hpp>
#include <app/rayceApp.hpp>
#include <core/input.hpp>
#include <core/timer.hpp>
#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/fence.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/renderPass.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/semaphore.hpp>
#include <vulkan/shaderModule.hpp>
#include <vulkan/surface.hpp>
#include <vulkan/swapchain.hpp>
#include <vulkan/window.hpp>

using namespace rayce;

RayceApp::RayceApp(const RayceOptions& options)
    : mCurrentFrame(0)
{
    mWindowWidth            = options.windowWidth;
    mWindowHeight           = options.windowHeight;
    mEnableValidationLayers = options.enableValidationLayers;

    pInput = std::make_shared<Input>();

    pWindow = std::make_unique<Window>(mWindowWidth, mWindowHeight, options.name, pInput);

    std::vector<const char*> windowExtensions = pWindow->getVulkanExtensions();
    pInstance                                 = std::make_unique<Instance>(mEnableValidationLayers, windowExtensions, mValidationLayers);

    pSurface = std::make_unique<Surface>(pInstance->getVkInstance(), pWindow->getNativeWindowHandle());

    bool raytracingSupported;
    mPhysicalDevice = pickPhysicalDevice(raytracingSupported);

    pDevice = std::make_unique<Device>(mPhysicalDevice, pSurface->getVkSurface(), pInstance->getEnabledValidationLayers(), raytracingSupported);
    pRTF    = std::make_unique<RTFunctions>(pDevice);

    pCommandPool = std::make_unique<CommandPool>(pDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    pFrameTimer = std::make_unique<Timer>();
    pFrameTimer->start();

    RAYCE_LOG_INFO("Created RayceApp!");
}

RayceApp::~RayceApp()
{
    RAYCE_LOG_INFO("Destroyed RayceApp!");
};

bool RayceApp::run()
{
    if (!onInitialize())
    {
        RAYCE_LOG_ERROR("onInitialize() failed!");
        return false;
    }

    recreateSwapchain();

    while (!pWindow->shouldClose())
    {
        pWindow->pollEvents();

        mFrametime = static_cast<float>(pFrameTimer->elapsedMicroseconds().count()) * 0.000001f; // We need the resolution.
        pFrameTimer->restart();

        onUpdate(mFrametime);

        onFrameDraw();
    }

    RAYCE_CHECK_VK(vkDeviceWaitIdle(pDevice->getVkDevice()), "Waiting for device idle failed!");

    if (!onShutdown())
    {
        RAYCE_LOG_ERROR("onShutdown() failed!");
        return false;
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

void RayceApp::onUpdate(float dt) { RAYCE_UNUSED(dt); }

void RayceApp::onFrameDraw()
{
    VkDevice logicalDevice   = pDevice->getVkDevice();
    VkSwapchainKHR swapchain = pSwapchain->getVkSwapchain();

    std::unique_ptr<Fence>& inFlightFence     = mInFlightFences[mCurrentFrame];
    const VkSemaphore imageAvailableSemaphore = mImageAvailableSemaphores[mCurrentFrame]->getVkSemaphore();
    const VkSemaphore renderFinishedSemaphore = mRenderFinishedSemaphores[mCurrentFrame]->getVkSemaphore();

    const uint64 noTimeout = std::numeric_limits<uint64>::max(); // uint64 max means no limits
    inFlightFence->wait(noTimeout);
    inFlightFence->reset();

    uint32 imageIndex;
    VkResult acquisitionResult = vkAcquireNextImageKHR(logicalDevice, swapchain, noTimeout, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
        return;
    }

    if (acquisitionResult != VK_SUCCESS && acquisitionResult != VK_SUBOPTIMAL_KHR)
    {
        RAYCE_ABORT("Acquisition of swap image failed!");
    }

    VkCommandBuffer commandBuffer = pCommandBuffers->beginCommandBuffer(imageIndex);

    onRender(commandBuffer, imageIndex);

    pImguiInterface->begin(pWindow);
    onImGuiRender(commandBuffer, imageIndex);
    std::vector<VkClearValue> clearValues; //{ { { 0.1f, 0.1f, 0.1f, 1.0f } } };
    pImguiInterface->end(commandBuffer, mSwapchainFramebuffers[imageIndex], clearValues);

    pCommandBuffers->endCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[]    = { renderFinishedSemaphore };
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;
    submitInfo.commandBufferCount     = 1;
    submitInfo.pCommandBuffers        = &commandBuffer;
    submitInfo.signalSemaphoreCount   = 1;
    submitInfo.pSignalSemaphores      = signalSemaphores;

    RAYCE_CHECK_VK(vkQueueSubmit(pDevice->getVkGraphicsQueue(), 1, &submitInfo, inFlightFence->getVkFence()), "Submit to graphics queue failed!");
    RAYCE_CHECK_VK(vkQueueWaitIdle(pDevice->getVkGraphicsQueue()), "vkQueueWaitIdle");

    pImguiInterface->platformWindows();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    VkSwapchainKHR swapchains[]    = { pSwapchain->getVkSwapchain() };
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    VkResult presentResult = vkQueuePresentKHR(pDevice->getVkPresentQueue(), &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
    }
    else if (presentResult != VK_SUCCESS)
    {
        RAYCE_ABORT("Presenting to queue failed!");
    }

    mCurrentFrame = (mCurrentFrame + 1) % mInFlightFences.size();
}

void RayceApp::onRender(VkCommandBuffer, const uint32)
{
    // UI rasterization is done later as overlay
}

void RayceApp::onImGuiRender(VkCommandBuffer, const uint32) {}

VkPhysicalDevice RayceApp::pickPhysicalDevice(bool& raytracingSupported)
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
            RAYCE_LOG_INFO("Geometry shader unavailable!");
            continue;
        }

        uint32 extensionCount;
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, deviceExtensions.data());

        bool rayTracingAvailable = std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                                               [](const VkExtensionProperties& extension)
                                               { return strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0; });
        rayTracingAvailable &= std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                                           [](const VkExtensionProperties& extension)
                                           { return strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0; });

        if (!rayTracingAvailable)
        {
            RAYCE_LOG_WARN("Raytracing unavailable - Application will not be able to trace rays - but RenderDoc can still work!");
        }

        const bool swapchainAvailable =
            std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [](const VkExtensionProperties& extension)
                        { return strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });

        if (!swapchainAvailable)
        {
            RAYCE_LOG_INFO("Swapchain unavailable!");
            continue;
        }

        uint32 queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyPropertyCount, queueFamilyProperties.data());

        const bool graphicsQueueFamilyAvailable =
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily)
                        { return (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT); });

        if (!graphicsQueueFamilyAvailable)
        {
            RAYCE_LOG_INFO("Graphics Queues unavailable!");
            continue;
        }

        const bool computeQueueFamilyAvailable =
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily)
                        { return (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT); });

        if (!computeQueueFamilyAvailable)
        {
            RAYCE_LOG_INFO("Compute Queues unavailable!");
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
            RAYCE_LOG_INFO("Present Queues unavailable!");
            continue;
        }

        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            RAYCE_LOG_WARN("Device is not a discrete GPU. This could result in really bad performance!");
        }

        pickedDevice        = candidate;
        raytracingSupported = rayTracingAvailable;
        RAYCE_LOG_INFO("Using device: %s!", deviceProperties.deviceName);
        break;
    }

    RAYCE_CHECK_NOTNULL(pickedDevice, "No compatible physical device available!");

    return pickedDevice;
}

void RayceApp::recreateSwapchain()
{
    while (pWindow->isMinimized())
    {
        pWindow->waitEvents();
    }

    VkDevice logicalDevice = pDevice->getVkDevice();
    vkDeviceWaitIdle(logicalDevice);

    pCommandBuffers.reset();
    pImguiInterface.reset();
    mSwapchainFramebuffers.clear();
    pRenderPass.reset();
    mInFlightFences.clear();
    mRenderFinishedSemaphores.clear();
    mImageAvailableSemaphores.clear();
    pSwapchain.reset();

    pSwapchain.reset(new Swapchain(pDevice, pSurface->getVkSurface(), pWindow->getNativeWindowHandle()));

    pRenderPass.reset(new RenderPass(pDevice, pSwapchain, VK_ATTACHMENT_LOAD_OP_CLEAR));

    for (const std::unique_ptr<ImageView>& imageView : pSwapchain->getImageViews())
    {
        mSwapchainFramebuffers.push_back(std::make_unique<Framebuffer>(pDevice, pSwapchain, pRenderPass, imageView));

        mImageAvailableSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mRenderFinishedSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mInFlightFences.push_back(std::make_unique<Fence>(pDevice, true));
    }

    // one command buffer per swapchain image
    int32 framesInFlight = pSwapchain->getImageCount();
    pCommandBuffers.reset(new CommandBuffers(pDevice, pCommandPool, framesInFlight));
    pImguiInterface.reset(new ImguiInterface(pInstance, pDevice, pCommandPool, pSwapchain, pWindow->getNativeWindowHandle()));
}
