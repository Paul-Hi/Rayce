/// @file      simpleGUI.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "simpleGUI.hpp"
#include <core.hpp>
#include <imgui.h>
#include <scene.hpp>
#include <vulkan.hpp>

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

    pScene = std::make_unique<RayceScene>();

    const str testScene = ".\\assets\\scenes\\testScene.xml";

    pScene->loadFromMitsubaFile(testScene, device, commandPool, 1.0f);

    auto& geometry = pScene->getGeometry();

    AccelerationStructureInitData accelerationStructureInitData{};
    accelerationStructureInitData.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    auto& triangleMeshes    = geometry->getTriangleMeshes();
    auto& proceduralSpheres = geometry->getProceduralSpheres();

    std::vector<std::vector<std::pair<VkTransformMatrixKHR, uint32>>> instanceInfo(triangleMeshes.size() + 1);

    for (ptr_size i = 0; i < triangleMeshes.size(); ++i)
    {
        const TriangleMeshGeometry& triMesh                   = triangleMeshes[i];
        accelerationStructureInitData.vertexDataDeviceAddress = triMesh.vertexBuffer->getDeviceAddress();
        accelerationStructureInitData.indexDataDeviceAddress  = triMesh.indexBuffer->getDeviceAddress();
        accelerationStructureInitData.maxVertex               = triMesh.maxVertex;
        accelerationStructureInitData.primitiveCount          = triMesh.primitiveCount;
        accelerationStructureInitData.procedural              = false;
        mBLAS.push_back(std::make_unique<AccelerationStructure>(device, commandPool, accelerationStructureInitData));

        for (ptr_size j = 0; j < triMesh.transformationMatrices.size(); ++j)
        {
            auto instance             = std::make_unique<InstanceData>();
            instance->materialId      = triMesh.materialId;
            instance->lightId         = -1; // triMesh.lightId;
            instance->indexReference  = accelerationStructureInitData.indexDataDeviceAddress;
            instance->vertexReference = accelerationStructureInitData.vertexDataDeviceAddress;
            instance->sphereId        = -1;
            mInstances.push_back(std::move(instance));

            const auto& tr = triMesh.transformationMatrices[j];

            VkTransformMatrixKHR transformationMatrix = {
                tr(0, 0), tr(0, 1), tr(0, 2), tr(0, 3),
                tr(1, 0), tr(1, 1), tr(1, 2), tr(1, 3),
                tr(2, 0), tr(2, 1), tr(2, 2), tr(2, 3)
            };
            instanceInfo[i].push_back({ transformationMatrix, 0 });
        }
    }

    if (!proceduralSpheres.empty())
    {
        // for now put all spheres in one BLAS - should be more efficent since we do not have more complex or moving stuff.
        // FIXME: The buffers should live somewhere else...
        std::vector<AxisAlignedBoundingBox> aabbs;
        for (ptr_size i = 0; i < proceduralSpheres.size(); ++i)
        {
            const ProceduralSphereGeometry& sphere = proceduralSpheres[i];
            mSpheres.push_back(*sphere.sphere);
            aabbs.push_back(*sphere.boundingBox);
        }

        mAABBBuffer = std::make_unique<Buffer>(device, sizeof(AxisAlignedBoundingBox) * aabbs.size(),
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        mAABBBuffer->allocateMemory(device, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        Buffer::uploadDataWithStagingBuffer(device, commandPool, *mAABBBuffer, aabbs);

        accelerationStructureInitData.aabbDataDeviceAddress = mAABBBuffer->getDeviceAddress();
        accelerationStructureInitData.aabbStride            = sizeof(AxisAlignedBoundingBox);
        accelerationStructureInitData.primitiveCount        = mSpheres.size();
        accelerationStructureInitData.procedural            = true;

        auto tr = mat4::Identity();

        VkTransformMatrixKHR transformationMatrix = {
            tr(0, 0), tr(0, 1), tr(0, 2), tr(0, 3),
            tr(1, 0), tr(1, 1), tr(1, 2), tr(1, 3),
            tr(2, 0), tr(2, 1), tr(2, 2), tr(2, 3)
        };
        instanceInfo[mBLAS.size()].push_back({ transformationMatrix, 1 });
        mBLAS.push_back(std::make_unique<AccelerationStructure>(device, commandPool, accelerationStructureInitData));

        for (ptr_size i = 0; i < proceduralSpheres.size(); ++i)
        {
            const ProceduralSphereGeometry& sphere = proceduralSpheres[i];

            for (ptr_size j = 0; j < sphere.transformationMatrices.size(); ++j)
            {
                auto instance             = std::make_unique<InstanceData>();
                instance->materialId      = sphere.materialId;
                instance->lightId         = sphere.lightId;
                instance->indexReference  = 0;
                instance->vertexReference = 0;
                instance->sphereId        = i;
                mInstances.push_back(std::move(instance));
            }
        }
    }

    accelerationStructureInitData.type           = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureInitData.primitiveCount = 0;
    uint32 i                                     = 0;
    for (const std::unique_ptr<AccelerationStructure>& blas : mBLAS)
    {
        for (const auto& [transformationMatrix, hitGroup] : instanceInfo[i])
        {
            accelerationStructureInitData.transformMatrices.push_back(transformationMatrix);
            accelerationStructureInitData.blasDeviceAddresses.push_back({ blas->getDeviceAddress(), hitGroup });
            accelerationStructureInitData.primitiveCount++;
        }
        i++;
    }
    pTLAS = std::make_unique<AccelerationStructure>(device, commandPool, accelerationStructureInitData);

    // camera
    float aspect = static_cast<float>(getWindowWidth()) / static_cast<float>(getWindowHeight());
    pCamera      = std::make_unique<Camera>(aspect, 45.0f, 0.01f, 100.0f, vec3(0.0f, 1.0f, 8.0f), vec3(0.0f, 0.5f, 0.0f), getInput());

    mAccumulationFrame = 0;

    return true;
}

bool SimpleGUI::onShutdown()
{
    return RayceApp::onShutdown();
}

void SimpleGUI::onUpdate(float dt)
{
    RayceApp::onUpdate(dt);

    bool cameraMoved = pCamera->update(dt);

    if (cameraMoved)
    {
        CameraDataRT cameraDataRT;
        cameraDataRT.inverseView       = pCamera->getInverseView();
        cameraDataRT.inverseProjection = pCamera->getInverseProjection();

        pRaytracingPipeline->updateCameraData(cameraDataRT);

        mAccumulationFrame = 0;
    }
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
    subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

    const std::vector<std::unique_ptr<ImageView>>& swapchainImageViews = getSwapchain()->getImageViews();
    const VkImage swapchainImage                                       = swapchainImageViews[imageIndex]->getVkImage();
    const VkImage rtImage                                              = pRaytracingTargetImage->getVkImage();

    ImageMemoryBarrier::Create(commandBuffer, rtImage, subresourceRange, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    const uint32 alignedHandleSize = pRaytracingPipeline->getAlignedHandleSize();

    const auto& [rayGenOffset, rayGenSize] = pRaytracingPipeline->getRayGenAddress();
    const auto& [chitOffset, chitSize] = pRaytracingPipeline->getClosestHitAddress();
    const auto& [missOffset, missSize] = pRaytracingPipeline->getMissAddress();

    VkStridedDeviceAddressRegionKHR raygenEntry{};
    raygenEntry.deviceAddress = rayGenOffset;
    raygenEntry.stride        = alignedHandleSize;
    raygenEntry.size          = rayGenSize;

    VkStridedDeviceAddressRegionKHR hitEntry{};
    hitEntry.deviceAddress = chitOffset;
    hitEntry.stride        = alignedHandleSize;
    hitEntry.size          = chitSize;

    VkStridedDeviceAddressRegionKHR missEntry{};
    missEntry.deviceAddress = missOffset;
    missEntry.stride        = alignedHandleSize;
    missEntry.size          = missSize;

    VkStridedDeviceAddressRegionKHR callableEntry{};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pRaytracingPipeline->getVkPipeline());
    std::vector<VkDescriptorSet> rtDescriptorSets = pRaytracingPipeline->getVkDescriptorSets(imageIndex);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pRaytracingPipeline->getVkPipelineLayout(), 0, rtDescriptorSets.size(), rtDescriptorSets.data(), 0, nullptr);

    struct
    {
        int32 frame;
        int32 lightCount;
    } pushConstants;
    pushConstants.frame      = mAccumulationFrame;
    pushConstants.lightCount = pScene->getLights().size();

    vkCmdPushConstants(commandBuffer, pRaytracingPipeline->getVkPipelineLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(int32) * 2, static_cast<void*>(&pushConstants));

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

    mAccumulationFrame++;
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

    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin("Frametime Overlay", nullptr, window_flags))
    {
        ImGui::Spacing();
        double fr = static_cast<double>(ImGui::GetIO().Framerate); // cast since text implicitely casts as well -> Warning.
        ImGui::Text("Average %.3f ms/frame", 1000.0 / fr);
        ImGui::End();
    }
}

void SimpleGUI::recreateSwapchain()
{
    RayceApp::recreateSwapchain();

    auto& swapchain   = getSwapchain();
    auto& device      = getDevice();
    auto& commandPool = getCommandPool();

    VkFormat format        = swapchain->getSurfaceFormat().format;
    pRaytracingTargetImage = std::make_unique<Image>(device, swapchain->getSwapExtent(), format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    pRaytracingTargetImage->allocateMemory(device, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    pRaytracingTargetView = std::make_unique<ImageView>(device, *pRaytracingTargetImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    auto& textureViews    = pScene->getTextureViews();
    auto& samplers        = pScene->getSamplers();

    VkExtent2D extent = swapchain->getSwapExtent();
    float aspect      = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    pCamera->updateAspect(aspect);
    CameraDataRT cameraDataRT;
    cameraDataRT.inverseView       = pCamera->getInverseView();
    cameraDataRT.inverseProjection = pCamera->getInverseProjection();
    pRaytracingPipeline.reset(new RaytracingPipeline(device, commandPool, swapchain, pTLAS, cameraDataRT, static_cast<uint32>(textureViews.size()), pRaytracingTargetView, swapchain->getImageCount()));

    pRaytracingPipeline->updateModelData(device, mInstances, mSpheres, pScene->getMaterials(), pScene->getLights(), textureViews, samplers);
}

int main(int argc, char** argv)
{
    RAYCE_UNUSED(argc);
    RAYCE_UNUSED(argv);

    RayceOptions options;
    options.windowWidth  = 1920;
    options.windowHeight = 1080;
    options.name         = "Rayce GUI";
#ifdef RAYCE_DEBUG
    options.enableValidationLayers = true;
#else
    options.enableValidationLayers = false;
#endif

    SimpleGUI application(options);

    application.run();

    return 0;
}
