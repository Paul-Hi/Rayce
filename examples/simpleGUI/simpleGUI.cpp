/// @file      simpleGUI.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "simpleGUI.hpp"
#include <imgui.h>
#include <scene/rayceScene.hpp>
#include <vulkan/accelerationStructure.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageMemoryBarrier.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/raytracingPipeline.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/swapchain.hpp>

using namespace rayce;

SimpleGUI::SimpleGUI(const RayceOptions& options)
    : RayceApp::RayceApp(options)
{
}

bool SimpleGUI::onInitialize()
{
    if (!RayceApp::onInitialize())
        return false;

    auto& device      = getDevice();
    auto& commandPool = getCommandPool();

    // Test geometry

    pScene = std::make_unique<RayceScene>();

    const str cVikingRoomObj = ".\\assets\\obj\\viking_room.obj";

    pScene->loadFromObj(cVikingRoomObj, device, commandPool);

    auto& geometry = pScene->getGeometry();

    AccelerationStructureInitData accelerationStructureInitData{};
    accelerationStructureInitData.type                    = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureInitData.vertexDataDeviceAddress = geometry->getVertexBuffer()->getDeviceAddress();
    accelerationStructureInitData.indexDataDeviceAddress  = geometry->getIndexBuffer()->getDeviceAddress();
    accelerationStructureInitData.maxVertex               = pScene->maxVertex();
    accelerationStructureInitData.primitiveCount          = pScene->primitiveCount();
    mBLAS.push_back(std::make_unique<AccelerationStructure>(device, commandPool, accelerationStructureInitData));

    accelerationStructureInitData.type           = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureInitData.primitiveCount = 1;
    for (const std::unique_ptr<AccelerationStructure>& blas : mBLAS)
    {
        accelerationStructureInitData.blasDeviceAddresses.push_back(blas->getDeviceAddress());
    }
    pTLAS = std::make_unique<AccelerationStructure>(device, commandPool, accelerationStructureInitData);

    return true;
}

bool SimpleGUI::onShutdown()
{
    return RayceApp::onShutdown();
}

void SimpleGUI::onUpdate()
{
    RayceApp::onUpdate();
}

void SimpleGUI::onFrameDraw()
{
    RayceApp::onFrameDraw();
}

void SimpleGUI::onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    RayceApp::onRender(commandBuffer, imageIndex);

    // raytracing
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;

    const std::vector<std::unique_ptr<ImageView>>& swapchainImageViews = getSwapchain()->getImageViews();
    const VkImage swapchainImage                                       = swapchainImageViews[imageIndex]->getVkImage();
    const VkImage rtImage                                              = pRaytracingTargetImage->getVkImage();

    ImageMemoryBarrier::Create(commandBuffer, rtImage, subresourceRange, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    const uint32 alignedHandleSize = pRaytracingPipeline->getAlignedHandleSize();

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
    std::vector<VkDescriptorSet> rtDescriptorSets = pRaytracingPipeline->getVkDescriptorSets(imageIndex);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pRaytracingPipeline->getVkPipelineLayout(), 0, rtDescriptorSets.size(), rtDescriptorSets.data(), 0, nullptr);

    struct BufferReferences
    {
        uint64 vertices;
        uint64 indices;
    } bufferReferences;

    auto& geometry = pScene->getGeometry();

    bufferReferences.vertices = geometry->getVertexBuffer()->getDeviceAddress();
    bufferReferences.indices  = geometry->getIndexBuffer()->getDeviceAddress();

    vkCmdPushConstants(commandBuffer, pRaytracingPipeline->getVkPipelineLayout(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(uint64) * 2, &bufferReferences);

    VkExtent2D extent = getSwapchain()->getSwapExtent();
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
}

void SimpleGUI::onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    RayceApp::onImGuiRender(commandBuffer, imageIndex);

    pScene->onImGuiRender();

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
    double fr = static_cast<double>(ImGui::GetIO().Framerate); // cast since text implicitely casts as well -> Warning.
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0 / fr, fr);
    ImGui::End();
}

void SimpleGUI::recreateSwapchain()
{
    RayceApp::recreateSwapchain();

    auto& swapchain = getSwapchain();
    auto& device    = getDevice();

    VkFormat format        = swapchain->getSurfaceFormat().format;
    pRaytracingTargetImage = std::make_unique<Image>(device, swapchain->getSwapExtent(), format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    pRaytracingTargetImage->allocateMemory(device, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    pRaytracingTargetView = std::make_unique<ImageView>(device, *pRaytracingTargetImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    pRaytracingPipeline.reset(new RaytracingPipeline(device, swapchain, pTLAS, pRaytracingTargetView, static_cast<uint32>(swapchain->getImageViews().size())));

    pRaytracingPipeline->updateImageView(pScene->getTextureView(0));

}

int main(int argc, char** argv)
{
    RAYCE_UNUSED(argc);
    RAYCE_UNUSED(argv);

    RayceOptions options;
    options.windowWidth  = 1920;
    options.windowHeight = 1080;
    options.name         = "SimpleGUI";
#ifdef RAYCE_DEBUG
    options.enableValidationLayers = true;
#else
    options.enableValidationLayers = false;
#endif

    SimpleGUI application(options);

    application.run();

    return 0;
}
