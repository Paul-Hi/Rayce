/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <imgui.h>
#include <imguiInterface.hpp>
#include <rayceApp.hpp>
#include <vulkan/accelerationStructure.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandBuffers.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/fence.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/graphicsPipeline.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageMemoryBarrier.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/raytracingPipeline.hpp>
#include <vulkan/renderPass.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/semaphore.hpp>
#include <vulkan/shaderModule.hpp>
#include <vulkan/surface.hpp>
#include <vulkan/swapchain.hpp>
#include <vulkan/vertex.hpp>
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
    mPhysicalDevice = pickPhysicalDevice(raytracingSupported);

    pDevice = std::make_unique<Device>(mPhysicalDevice, pSurface->getVkSurface(), pInstance->getEnabledValidationLayers(), raytracingSupported);
    pRTF    = std::make_unique<RTFunctions>(pDevice);

    pCommandPool = std::make_unique<CommandPool>(pDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    RAYCE_CHECK(onInitialize(), "onInitialize() failed!");

    recreateSwapchain();

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
    // Test geometry
    const std::vector<Vertex> vertices = { { { -0.25f, -0.25f, 0.25f } },  { { 0.25f, -0.25f, 0.25f } },  { { -0.25f, 0.25f, 0.25f } },  { { 0.25f, 0.25f, 0.25f } },
                                           { { -0.25f, -0.25f, -0.25f } }, { { 0.25f, -0.25f, -0.25f } }, { { -0.25f, 0.25f, -0.25f } }, { { 0.25f, 0.25f, -0.25f } } };

    const std::vector<uint32> indices = { 2, 6, 7, 2, 3, 7, 0, 4, 5, 0, 1, 5, 0, 2, 6, 0, 4, 6, 1, 3, 7, 1, 5, 7, 0, 2, 3, 0, 1, 3, 4, 6, 7, 4, 5, 7 };

    std::unique_ptr<Buffer> vertexBuffer = std::make_unique<Buffer>(pDevice, sizeof(Vertex) * vertices.size(),
                                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    std::unique_ptr<Buffer> indexBuffer  = std::make_unique<Buffer>(pDevice, sizeof(uint32) * indices.size(),
                                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

    vertexBuffer->allocateMemory(pDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->allocateMemory(pDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer::uploadDataWithStagingBuffer(pDevice, pCommandPool, *vertexBuffer, vertices);
    Buffer::uploadDataWithStagingBuffer(pDevice, pCommandPool, *indexBuffer, indices);

    pGeometry = std::make_unique<Geometry>(std::move(vertexBuffer), vertices.size(), std::move(indexBuffer), indices.size());

    AccelerationStructureInitData accelerationStructureInitData{};
    accelerationStructureInitData.type                    = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureInitData.vertexDataDeviceAddress = pGeometry->getVertexBuffer()->getDeviceAddress();
    accelerationStructureInitData.indexDataDeviceAddress  = pGeometry->getIndexBuffer()->getDeviceAddress();
    accelerationStructureInitData.maxVertex               = static_cast<uint32>(vertices.size() - 1);
    accelerationStructureInitData.primitiveCount          = static_cast<uint32>(indices.size() / 3);
    mBLAS.push_back(std::make_unique<AccelerationStructure>(pDevice, pCommandPool, accelerationStructureInitData));

    accelerationStructureInitData.type           = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureInitData.primitiveCount = 1;
    for (const std::unique_ptr<AccelerationStructure>& blas : mBLAS)
    {
        accelerationStructureInitData.blasDeviceAddresses.push_back(blas->getDeviceAddress());
    }
    pTLAS = std::make_unique<AccelerationStructure>(pDevice, pCommandPool, accelerationStructureInitData);

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
    VkResult acquisitionResult = vkAcquireNextImageKHR(logicalDevice, swapchain, noTimeout, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_SUBOPTIMAL_KHR) // || mWireframe != pGraphicsPipeline->Iswireframe())
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

    pImguiInterface->begin();
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

void RayceApp::onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    // raytracing

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;

    const std::vector<std::unique_ptr<ImageView>>& swapchainImageViews = pSwapchain->getImageViews();
    const VkImage swapchainImage                                       = swapchainImageViews[imageIndex]->getVkImage();
    const VkImage rtImage                                              = pRaytracingTargetImage->getVkImage();

    ImageMemoryBarrier::Create(commandBuffer, rtImage, subresourceRange, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    const uint32 alignedHandleSize = pRaytracingPipeline->getAlignedHandleSize();

    const std::unique_ptr<Buffer>& shaderBindingTableBuffer = pRaytracingPipeline->getShaderBindingTableBuffer();

    VkStridedDeviceAddressRegionKHR raygenEntry{};
    raygenEntry.deviceAddress = pRaytracingPipeline->getRayGenAddress();
    raygenEntry.stride        = alignedHandleSize;
    raygenEntry.size          = alignedHandleSize;

    VkStridedDeviceAddressRegionKHR hitEntry{};
    hitEntry.deviceAddress = pRaytracingPipeline->getClosestHitAddress();
    hitEntry.stride        = alignedHandleSize;
    hitEntry.size          = alignedHandleSize;

    VkStridedDeviceAddressRegionKHR missEntry{};
    missEntry.deviceAddress = pRaytracingPipeline->getMissAddress();
    missEntry.stride        = alignedHandleSize;
    missEntry.size          = alignedHandleSize;

    VkStridedDeviceAddressRegionKHR callableEntry{};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pRaytracingPipeline->getVkPipeline());
    VkDescriptorSet rtDescriptorSet = pRaytracingPipeline->getVkDescriptorSet(imageIndex);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pRaytracingPipeline->getVkPipelineLayout(), 0, 1, &rtDescriptorSet, 0, 0);

    VkExtent2D extent = pSwapchain->getSwapExtent();
    pRTF->vkCmdTraceRaysKHR(commandBuffer, &raygenEntry, &missEntry, &hitEntry, &callableEntry, extent.width, extent.height, 1);

    // copy image

    ImageMemoryBarrier::Create(commandBuffer, swapchainImage, subresourceRange, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ImageMemoryBarrier::Create(commandBuffer, rtImage, subresourceRange, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.srcOffset      = { 0, 0, 0 };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.dstOffset      = { 0, 0, 0 };
    copyRegion.extent         = { extent.width, extent.height, 1 };
    vkCmdCopyImage(commandBuffer, rtImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    ImageMemoryBarrier::Create(commandBuffer, swapchainImage, subresourceRange, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // UI rasterization is done later as overlay

    // std::array<VkClearValue, 1> clearValues = {};
    // clearValues[0].color                    = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    // VkRenderPassBeginInfo renderPassBeginInfo{};
    // renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // renderPassBeginInfo.renderPass        = pGraphicsPipeline->getRenderPass()->getVkRenderPass();
    // renderPassBeginInfo.framebuffer       = mSwapchainFramebuffers[imageIndex]->getVkFramebuffer();
    // renderPassBeginInfo.renderArea.offset = { 0, 0 };
    // renderPassBeginInfo.renderArea.extent = pSwapchain->getSwapExtent();
    // renderPassBeginInfo.clearValueCount   = static_cast<uint32>(clearValues.size());
    // renderPassBeginInfo.pClearValues      = clearValues.data();

    // vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    //{
    //     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->getVkPipeline());
    //
    //    //    VkBuffer vertexBuffers[] = { pGeometry->getVertexBuffer()->getVkBuffer() };
    //    VkDeviceSize offsets[]   = { 0 };
    //    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    //    vkCmdBindIndexBuffer(commandBuffer, pGeometry->getIndexBuffer()->getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
    //
    //    //    vkCmdDrawIndexed(commandBuffer, pGeometry->getIndexCount(), 1, 0, 0, 0);
    //}
    // vkCmdEndRenderPass(commandBuffer);
}

void RayceApp::onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoDocking;

    if (!ImGui::Begin("Rayce", nullptr, window_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

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
    pGraphicsPipeline.reset();
    mInFlightFences.clear();
    mRenderFinishedSemaphores.clear();
    mImageAvailableSemaphores.clear();
    pSwapchain.reset();

    pSwapchain.reset(new Swapchain(pDevice, pSurface->getVkSurface(), pWindow->getNativeWindowHandle()));

    pGraphicsPipeline.reset(new GraphicsPipeline(pDevice, pSwapchain, false));

    int32 swapchainImageCount = 0;
    for (const std::unique_ptr<ImageView>& imageView : pSwapchain->getImageViews())
    {
        swapchainImageCount++;
        mSwapchainFramebuffers.push_back(std::make_unique<Framebuffer>(pDevice, pSwapchain, pGraphicsPipeline->getRenderPass(), imageView));

        mImageAvailableSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mRenderFinishedSemaphores.push_back(std::make_unique<Semaphore>(pDevice));
        mInFlightFences.push_back(std::make_unique<Fence>(pDevice, true));
    }

    VkFormat format        = pSwapchain->getSurfaceFormat().format;
    pRaytracingTargetImage = std::make_unique<Image>(pDevice, pSwapchain->getSwapExtent(), format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    pRaytracingTargetImage->allocateMemory(pDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    pRaytracingTargetView = std::make_unique<ImageView>(pDevice, *pRaytracingTargetImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    pRaytracingPipeline.reset(new RaytracingPipeline(pDevice, pTLAS, pRaytracingTargetView, swapchainImageCount));

    // one command buffer per swapchain image
    pCommandBuffers.reset(new CommandBuffers(pDevice, pCommandPool, swapchainImageCount));
    pImguiInterface.reset(new ImguiInterface(pInstance, pDevice, pCommandPool, pSwapchain, pWindow->getNativeWindowHandle()));
}
