/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <rayceApp.hpp>
#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/fence.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/graphicsPipeline.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/renderPass.hpp>
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

    pWindow = std::make_unique<Window>(mWindowWidth, mWindowHeight, options.name);

    std::vector<const char*> windowExtensions = pWindow->getVulkanExtensions();
    pInstance                                 = std::make_unique<Instance>(mEnableValidationLayers, windowExtensions, mValidationLayers);

    pSurface = std::make_unique<Surface>(pInstance->getVkInstance(), pWindow->getNativeWindowHandle());

    bool raytracingSupported;
    VkPhysicalDevice physicalDevice = pickPhysicalDevice(raytracingSupported);

    pDevice = std::make_unique<Device>(physicalDevice, pSurface->getVkSurface(), pInstance->getEnabledValidationLayers(), raytracingSupported);

    pSwapchain = std::make_unique<Swapchain>(physicalDevice, pDevice, pSurface->getVkSurface(), pWindow->getNativeWindowHandle());

    pGraphicsPipeline = std::make_unique<GraphicsPipeline>(pDevice, pSwapchain, false);

    int32 swapchainImageCount = 0;
    for (const std::unique_ptr<ImageView>& imageView : pSwapchain->getImageViews())
    {
        swapchainImageCount++;
        mSwapchainFramebuffers.push_back(std::make_unique<Framebuffer>(pDevice, pSwapchain, pGraphicsPipeline->getRenderPass(), imageView));

        mImageAvailableSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mRenderFinishedSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mInFlightFences.push_back(std::make_unique<Fence>(pDevice, true));
    }

    pCommandPool = std::make_unique<CommandPool>(pDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // one command buffer per swapchain image (frames in flight?)
    pCommandBuffers = std::make_unique<CommandBuffers>(pDevice, pCommandPool, swapchainImageCount);

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
        onFrameDraw();
    }

    RAYCE_CHECK_VK(vkDeviceWaitIdle(pDevice->getVkDevice()), "Waiting for device idle failed!");

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
    RAYCE_CHECK_VK(vkAcquireNextImageKHR(logicalDevice, swapchain, noTimeout, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex), "Acquisition of swap image failed!");

    VkCommandBuffer commandBuffer = pCommandBuffers->beginCommandBuffer(imageIndex);

    onRender(commandBuffer, imageIndex);

    onImGuiRender(commandBuffer, imageIndex);

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

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    VkSwapchainKHR swapchains[]    = { pSwapchain->getVkSwapchain() };
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    RAYCE_CHECK_VK(vkQueuePresentKHR(pDevice->getVkPresentQueue(), &presentInfo), "Presenting to queue failed!");
}

void RayceApp::onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    std::array<VkClearValue, 1> clearValues = {};
    clearValues[0].color                    = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass        = pGraphicsPipeline->getRenderPass()->getVkRenderPass();
    renderPassBeginInfo.framebuffer       = mSwapchainFramebuffers[imageIndex]->getVkFramebuffer();
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = pSwapchain->getSwapExtent();
    renderPassBeginInfo.clearValueCount   = static_cast<uint32>(clearValues.size());
    renderPassBeginInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->getVkPipeline());
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffer);
}

void RayceApp::onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex) {}

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
                                               [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0; });
        rayTracingAvailable &= std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                                           [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0; });

        if (!rayTracingAvailable)
        {
            RAYCE_LOG_WARN("Raytracing unavailable - Application will not be able to trace rays - but RenderDoc can still work!");
        }

        const bool swapchainAvailable =
            std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });

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
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily) { return (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT); });

        if (!graphicsQueueFamilyAvailable)
        {
            RAYCE_LOG_INFO("Graphics Queues unavailable!");
            continue;
        }

        const bool computeQueueFamilyAvailable =
            std::any_of(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& queueFamily) { return (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT); });

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
