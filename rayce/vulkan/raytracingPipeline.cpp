/// @file      raytracingPipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/utils.hpp>
#include <host_device.hpp>
#include <vulkan/accelerationStructure.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/descriptorPool.hpp>
#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/descriptorSets.hpp>
#include <vulkan/device.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/raytracingPipeline.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/sampler.hpp>
#include <vulkan/shaderModule.hpp>
#include <vulkan/swapchain.hpp>

using namespace rayce;

RaytracingPipeline::RaytracingPipeline(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const std::unique_ptr<class Swapchain>& swapchain, const std::unique_ptr<AccelerationStructure>& tlas, CameraDataRT& cameraData, uint32 requiredImageDescriptors, const std::unique_ptr<ImageView>& outputImage,
                                       uint32 framesInFlight)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mFramesInFlight(framesInFlight)
{
    pRTF = std::make_unique<RTFunctions>(logicalDevice);

    // accumulation image
    pAccumulationImage = std::make_unique<Image>(logicalDevice, swapchain->getSwapExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT);
    pAccumulationImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    pAccumulationImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_GENERAL);
    pAccumulationImageView = std::make_unique<ImageView>(logicalDevice, *pAccumulationImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

    VkDescriptorSetLayoutBinding layoutBindingDescriptorTLAS{};
    layoutBindingDescriptorTLAS.binding         = 0;
    layoutBindingDescriptorTLAS.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    layoutBindingDescriptorTLAS.descriptorCount = 1;
    layoutBindingDescriptorTLAS.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding layoutBindingDescriptorAccumulationImage{};
    layoutBindingDescriptorAccumulationImage.binding         = 1;
    layoutBindingDescriptorAccumulationImage.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindingDescriptorAccumulationImage.descriptorCount = 1;
    layoutBindingDescriptorAccumulationImage.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding layoutBindingDescriptorResultImage{};
    layoutBindingDescriptorResultImage.binding         = 2;
    layoutBindingDescriptorResultImage.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindingDescriptorResultImage.descriptorCount = 1;
    layoutBindingDescriptorResultImage.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { layoutBindingDescriptorTLAS, layoutBindingDescriptorAccumulationImage, layoutBindingDescriptorResultImage };

    pDescriptorSetLayoutRT = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);

    VkDescriptorSetLayoutBinding layoutBindingDescriptorCameraBuffer{};
    layoutBindingDescriptorCameraBuffer.binding         = 0;
    layoutBindingDescriptorCameraBuffer.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindingDescriptorCameraBuffer.descriptorCount = 1;
    layoutBindingDescriptorCameraBuffer.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings = { layoutBindingDescriptorCameraBuffer };

    pDescriptorSetLayoutCamera = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);

    VkDescriptorSetLayoutBinding layoutBindingDescriptorTextures{};
    layoutBindingDescriptorTextures.binding         = 0;
    layoutBindingDescriptorTextures.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindingDescriptorTextures.descriptorCount = requiredImageDescriptors;
    layoutBindingDescriptorTextures.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding layoutBindingDescriptorInstanceDataBuffer{};
    layoutBindingDescriptorInstanceDataBuffer.binding         = 1;
    layoutBindingDescriptorInstanceDataBuffer.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindingDescriptorInstanceDataBuffer.descriptorCount = 1;
    layoutBindingDescriptorInstanceDataBuffer.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding layoutBindingDescriptorMaterialDataBuffer{};
    layoutBindingDescriptorMaterialDataBuffer.binding         = 2;
    layoutBindingDescriptorMaterialDataBuffer.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindingDescriptorMaterialDataBuffer.descriptorCount = 1;
    layoutBindingDescriptorMaterialDataBuffer.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings = { layoutBindingDescriptorTextures, layoutBindingDescriptorInstanceDataBuffer, layoutBindingDescriptorMaterialDataBuffer };

    pDescriptorSetLayoutModel = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);

    VkDescriptorSetLayout descriptorSetLayoutRT     = pDescriptorSetLayoutRT->getVkDescriptorLayout();
    VkDescriptorSetLayout descriptorSetLayoutCamera = pDescriptorSetLayoutCamera->getVkDescriptorLayout();
    VkDescriptorSetLayout descriptorSetLayoutModel  = pDescriptorSetLayoutModel->getVkDescriptorLayout();
    VkDescriptorSetLayout setLayouts[]              = { descriptorSetLayoutRT, descriptorSetLayoutCamera, descriptorSetLayoutModel };

    VkPushConstantRange bufferReferencePushConstantRange{};
    bufferReferencePushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bufferReferencePushConstantRange.offset     = 0;
    bufferReferencePushConstantRange.size       = sizeof(int32);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = 3;
    pipelineLayoutCreateInfo.pSetLayouts            = setLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &bufferReferencePushConstantRange;

    RAYCE_CHECK_VK(vkCreatePipelineLayout(mVkLogicalDeviceRef, &pipelineLayoutCreateInfo, nullptr, &mVkPipelineLayout), "Creating pipeline layout failed!");

    // shader stages
    pRayGenShader     = std::make_unique<ShaderModule>(logicalDevice, ".\\assets\\shaders\\raygen.rgen.spv");
    pClosestHitShader = std::make_unique<ShaderModule>(logicalDevice, ".\\assets\\shaders\\closestHit.rchit.spv");
    pMissShader       = std::make_unique<ShaderModule>(logicalDevice, ".\\assets\\shaders\\miss.rmiss.spv");

    VkPipelineShaderStageCreateInfo shaderStages[] = { pRayGenShader->createShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR), pClosestHitShader->createShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR),
                                                       pMissShader->createShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR) };

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCreateInfos;

    VkRayTracingShaderGroupCreateInfoKHR rtShaderGroupCreateInfo{};
    rtShaderGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rtShaderGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rtShaderGroupCreateInfo.generalShader      = 0;
    rtShaderGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
    rtShaderGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
    rtShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos.push_back(rtShaderGroupCreateInfo);

    rtShaderGroupCreateInfo.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rtShaderGroupCreateInfo.closestHitShader = 1;
    rtShaderGroupCreateInfo.generalShader    = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos.push_back(rtShaderGroupCreateInfo);

    rtShaderGroupCreateInfo.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rtShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    rtShaderGroupCreateInfo.generalShader    = 2;
    shaderGroupCreateInfos.push_back(rtShaderGroupCreateInfo);

    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
    rayTracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineCreateInfo.stageCount                   = 3;
    rayTracingPipelineCreateInfo.pStages                      = shaderStages;
    rayTracingPipelineCreateInfo.groupCount                   = static_cast<uint32>(shaderGroupCreateInfos.size());
    rayTracingPipelineCreateInfo.pGroups                      = shaderGroupCreateInfos.data();
    rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineCreateInfo.layout                       = mVkPipelineLayout;
    RAYCE_CHECK_VK(pRTF->vkCreateRayTracingPipelinesKHR(mVkLogicalDeviceRef, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, nullptr, &mVkPipeline),
                   "Creating raytracing pipeline failed!");

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{};
    physicalDeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 physicalDeviceProperties2{};
    physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physicalDeviceProperties2.pNext = &physicalDeviceRayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(logicalDevice->getVkPhysicalDevice(), &physicalDeviceProperties2);

    VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures{};
    physicalDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.pNext = &physicalDeviceAccelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(logicalDevice->getVkPhysicalDevice(), &physicalDeviceFeatures2);

    // shader binding table
    mAlignedHandleSize                  = quickAlign(physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize, physicalDeviceRayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32 baseAlignedHandleSize  = quickAlign(physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize, physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment);
    const uint32 groupCount             = rayTracingPipelineCreateInfo.groupCount;
    const uint32 shaderBindingTableSize = groupCount * baseAlignedHandleSize;
    mRayGenOffset                       = 0;
    mCHitOffset                         = baseAlignedHandleSize;
    mMissOffset                         = mCHitOffset + baseAlignedHandleSize;

    pShaderBindingTableBuffer =
        std::make_unique<Buffer>(logicalDevice, shaderBindingTableSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    pShaderBindingTableBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    std::vector<byte> shaderHandleTempStorage(groupCount * mAlignedHandleSize);
    RAYCE_CHECK_VK(pRTF->vkGetRayTracingShaderGroupHandlesKHR(mVkLogicalDeviceRef, mVkPipeline, 0, groupCount, shaderBindingTableSize, shaderHandleTempStorage.data()),
                   "Getting ray tracing shader group handles failed!");
    byte* mappedMemory = static_cast<byte*>(pShaderBindingTableBuffer->getDeviceMemory()->map(0, shaderBindingTableSize));
    std::memcpy(mappedMemory, shaderHandleTempStorage.data(), mAlignedHandleSize);
    mappedMemory += baseAlignedHandleSize;
    std::memcpy(mappedMemory, shaderHandleTempStorage.data() + mAlignedHandleSize, mAlignedHandleSize);
    mappedMemory += baseAlignedHandleSize;
    std::memcpy(mappedMemory, shaderHandleTempStorage.data() + 2 * mAlignedHandleSize, mAlignedHandleSize);
    pShaderBindingTableBuffer->getDeviceMemory()->unmap();

    // descriptor sets
    std::vector<VkDescriptorPoolSize> poolSizes({ { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 }, { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 }, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } });
    pDescriptorPool = std::make_unique<DescriptorPool>(logicalDevice, poolSizes, 1000 * poolSizes.size(), 0);

    pDescriptorSetsRT     = std::make_unique<DescriptorSets>(logicalDevice, pDescriptorPool, pDescriptorSetLayoutRT, framesInFlight);
    pDescriptorSetsCamera = std::make_unique<DescriptorSets>(logicalDevice, pDescriptorPool, pDescriptorSetLayoutCamera, framesInFlight);
    pDescriptorSetsModel  = std::make_unique<DescriptorSets>(logicalDevice, pDescriptorPool, pDescriptorSetLayoutModel, framesInFlight);

    // buffers
    mCameraBuffers.resize(framesInFlight);
    mCameraBuffersMapped.resize(framesInFlight);

    uint32 bufferSize = sizeof(CameraDataRT);

    for (ptr_size i = 0; i < framesInFlight; ++i)
    {
        mCameraBuffers[i] = std::make_unique<Buffer>(logicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        mCameraBuffers[i]->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        const std::unique_ptr<DeviceMemory>& deviceMemory = mCameraBuffers[i]->getDeviceMemory();

        mCameraBuffersMapped[i] = deviceMemory->map(0, bufferSize); // persistent mapping
    }

    // Write stuff
    VkAccelerationStructureKHR vkTLAS = tlas->getVkAccelerationStructure();
    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures    = &vkTLAS;

    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelerationStructureWrite.dstBinding      = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;

    VkDescriptorImageInfo descriptorAccumImageInfo{};
    descriptorAccumImageInfo.imageView   = pAccumulationImageView->getVkImageView();
    descriptorAccumImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet accumImageWrite{};
    accumImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accumImageWrite.dstBinding      = 1;
    accumImageWrite.descriptorCount = 1;
    accumImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    accumImageWrite.pImageInfo      = &descriptorAccumImageInfo;

    VkDescriptorImageInfo descriptorResultImageInfo{};
    descriptorResultImageInfo.imageView   = outputImage->getVkImageView();
    descriptorResultImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet resultImageWrite{};
    resultImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultImageWrite.dstBinding      = 2;
    resultImageWrite.descriptorCount = 1;
    resultImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageWrite.pImageInfo      = &descriptorResultImageInfo;

    VkDescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range  = bufferSize;

    VkWriteDescriptorSet cameraBufferWrite{};
    cameraBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    cameraBufferWrite.dstBinding      = 0;
    cameraBufferWrite.descriptorCount = 1;
    cameraBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    // uniform buffer update
    for (ptr_size i = 0; i < framesInFlight; ++i)
    {
        accelerationStructureWrite.dstSet                     = pDescriptorSetsRT->operator[](static_cast<uint32>(i));
        accumImageWrite.dstSet                                = pDescriptorSetsRT->operator[](static_cast<uint32>(i));
        resultImageWrite.dstSet                               = pDescriptorSetsRT->operator[](static_cast<uint32>(i));
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = { accelerationStructureWrite, accumImageWrite, resultImageWrite };
        pDescriptorSetsRT->update(writeDescriptorSets);

        memcpy(mCameraBuffersMapped[i], &cameraData, bufferSize);
        cameraBufferInfo.buffer       = mCameraBuffers[i]->getVkBuffer();
        cameraBufferWrite.pBufferInfo = &cameraBufferInfo;
        cameraBufferWrite.dstSet      = pDescriptorSetsCamera->operator[](static_cast<uint32>(i));
        writeDescriptorSets           = { cameraBufferWrite };
        pDescriptorSetsCamera->update(writeDescriptorSets);
    }

    RAYCE_LOG_INFO("Created raytracing pipeline!");
}

void RaytracingPipeline::updateModelData(const std::unique_ptr<Device>& logicalDevice, const std::vector<std::unique_ptr<InstanceData>>& instances, const std::vector<std::unique_ptr<Material>>& materials, const std::vector<std::unique_ptr<ImageView>>& images, const std::vector<std::unique_ptr<Sampler>>& samplers)
{
    // FIXME: If we want to update that some time in the future we should not create the buffers each time :D
    // buffers
    mInstanceBuffers.resize(mFramesInFlight);
    mInstanceBuffersMapped.resize(mFramesInFlight);
    mMaterialBuffers.resize(mFramesInFlight);
    mMaterialBuffersMapped.resize(mFramesInFlight);

    uint32 bufferSizeI = sizeof(InstanceData) * instances.size();
    uint32 bufferSizeM = sizeof(Material) * materials.size();

    for (ptr_size i = 0; i < mFramesInFlight; ++i)
    {
        mInstanceBuffers[i] = std::make_unique<Buffer>(logicalDevice, bufferSizeI, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        mInstanceBuffers[i]->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        mMaterialBuffers[i] = std::make_unique<Buffer>(logicalDevice, bufferSizeM, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        mMaterialBuffers[i]->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        const std::unique_ptr<DeviceMemory>& deviceMemoryI = mInstanceBuffers[i]->getDeviceMemory();
        const std::unique_ptr<DeviceMemory>& deviceMemoryM = mMaterialBuffers[i]->getDeviceMemory();

        mInstanceBuffersMapped[i] = deviceMemoryI->map(0, bufferSizeI); // persistent mapping
        mMaterialBuffersMapped[i] = deviceMemoryM->map(0, bufferSizeM); // persistent mapping
    }

    VkDescriptorImageInfo textureInfo{};
    textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet textureWrite{};
    textureWrite.sType          = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrite.dstBinding     = 0;
    textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    std::vector<VkDescriptorImageInfo> textureInfos;
    for (ptr_size i = 0; i < images.size(); ++i)
    {
        textureInfo.imageView = images[i]->getVkImageView();
        textureInfo.sampler   = samplers[i]->getVkSampler();
        textureInfos.push_back(textureInfo);
    }
    textureWrite.descriptorCount = textureInfos.size();
    textureWrite.pImageInfo      = textureInfos.data();

    VkDescriptorBufferInfo instanceBufferInfo{};
    instanceBufferInfo.offset = 0;
    instanceBufferInfo.range  = bufferSizeI;

    VkWriteDescriptorSet instanceBufferWrite{};
    instanceBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    instanceBufferWrite.dstBinding      = 1;
    instanceBufferWrite.descriptorCount = 1;
    instanceBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.offset = 0;
    materialBufferInfo.range  = bufferSizeM;

    VkWriteDescriptorSet materialBufferWrite{};
    materialBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    materialBufferWrite.dstBinding      = 2;
    materialBufferWrite.descriptorCount = 1;
    materialBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    for (ptr_size i = 0; i < mFramesInFlight; ++i)
    {
        textureWrite.dstSet = pDescriptorSetsModel->operator[](static_cast<uint32>(i));

        for (ptr_size j = 0; j < instances.size(); ++j)
        {
            InstanceData& deviceInstance = *(reinterpret_cast<InstanceData*>(mInstanceBuffersMapped[i]) + j);
            deviceInstance               = *instances[j].get();
        }

        for (ptr_size j = 0; j < materials.size(); ++j)
        {
            Material& deviceMaterial = *(reinterpret_cast<Material*>(mMaterialBuffersMapped[i]) + j);
            deviceMaterial           = *materials[j].get();
        }

        instanceBufferInfo.buffer       = mInstanceBuffers[i]->getVkBuffer();
        instanceBufferWrite.pBufferInfo = &instanceBufferInfo;
        instanceBufferWrite.dstSet      = pDescriptorSetsModel->operator[](static_cast<uint32>(i));

        materialBufferInfo.buffer       = mMaterialBuffers[i]->getVkBuffer();
        materialBufferWrite.pBufferInfo = &materialBufferInfo;
        materialBufferWrite.dstSet      = pDescriptorSetsModel->operator[](static_cast<uint32>(i));

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = { textureWrite, instanceBufferWrite, materialBufferWrite };
        pDescriptorSetsModel->update(writeDescriptorSets);
    }
}

void RaytracingPipeline::updateCameraData(CameraDataRT& cameraData)
{
    uint32 bufferSize = sizeof(CameraDataRT);

    // uniform buffer update
    for (ptr_size i = 0; i < mFramesInFlight; ++i)
    {
        memcpy(mCameraBuffersMapped[i], &cameraData, bufferSize);
    }
}

std::vector<VkDescriptorSet> RaytracingPipeline::getVkDescriptorSets(uint32 idx) const
{
    return { pDescriptorSetsRT->operator[](idx), pDescriptorSetsCamera->operator[](idx), pDescriptorSetsModel->operator[](idx) };
}

RaytracingPipeline::~RaytracingPipeline()
{
    pDescriptorSetsRT.reset();
    pDescriptorSetsCamera.reset();
    pDescriptorSetsModel.reset();
    pDescriptorSetLayoutRT.reset();
    pDescriptorSetLayoutCamera.reset();
    pDescriptorSetLayoutModel.reset();
    pDescriptorPool.reset();
    if (mVkPipeline)
    {
        vkDestroyPipeline(mVkLogicalDeviceRef, mVkPipeline, nullptr);
    }
    if (mVkPipelineLayout)
    {
        vkDestroyPipelineLayout(mVkLogicalDeviceRef, mVkPipelineLayout, nullptr);
    }
}
