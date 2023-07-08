/// @file      raytracingPipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/utils.hpp>
#include <vulkan/accelerationStructure.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/descriptorPool.hpp>
#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/descriptorSets.hpp>
#include <vulkan/device.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/raytracingPipeline.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/shaderModule.hpp>

using namespace rayce;

RaytracingPipeline::RaytracingPipeline(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<AccelerationStructure>& tlas, const std::unique_ptr<ImageView>& outputImage,
                                       uint32 descriptorsInFlight)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mDescriptorsInFlight(descriptorsInFlight)
{
    pRTF = std::make_unique<RTFunctions>(logicalDevice);

    VkDescriptorSetLayoutBinding layoutBindingDescriptorTLAS{};
    layoutBindingDescriptorTLAS.binding         = 0;
    layoutBindingDescriptorTLAS.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    layoutBindingDescriptorTLAS.descriptorCount = 1;
    layoutBindingDescriptorTLAS.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding layoutBindingDescriptorTargetImage{};
    layoutBindingDescriptorTargetImage.binding         = 1;
    layoutBindingDescriptorTargetImage.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindingDescriptorTargetImage.descriptorCount = 1;
    layoutBindingDescriptorTargetImage.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { layoutBindingDescriptorTLAS, layoutBindingDescriptorTargetImage };

    pDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);

    VkDescriptorSetLayout descriptorSetLayout = pDescriptorSetLayout->getVkDescriptorLayout();
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

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
    std::vector<VkDescriptorPoolSize> poolSizes({ { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, descriptorsInFlight }, { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorsInFlight } });
    pDescriptorPool = std::make_unique<DescriptorPool>(logicalDevice, poolSizes, descriptorsInFlight, 0);

    pDescriptorSets = std::make_unique<DescriptorSets>(logicalDevice, pDescriptorPool, pDescriptorSetLayout, descriptorsInFlight);

    // Write stuff
    VkAccelerationStructureKHR vkTLAS = tlas->getVkAccelerationStructure();
    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures    = &vkTLAS;

    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageView   = outputImage->getVkImageView();
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelerationStructureWrite.dstBinding      = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;

    VkWriteDescriptorSet imageWrite{};
    imageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    imageWrite.dstBinding      = 1;
    imageWrite.descriptorCount = 1;
    imageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.pImageInfo      = &descriptorImageInfo;

    for (ptr_size i = 0; i < descriptorsInFlight; ++i)
    {
        accelerationStructureWrite.dstSet                     = pDescriptorSets->operator[](i);
        imageWrite.dstSet                                     = pDescriptorSets->operator[](i);
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = { accelerationStructureWrite, imageWrite };
        pDescriptorSets->update(writeDescriptorSets);
    }

    RAYCE_LOG_INFO("Created raytracing pipeline!");
}

VkDescriptorSet RaytracingPipeline::getVkDescriptorSet(uint32 idx) const
{
    return pDescriptorSets->operator[](idx);
}

RaytracingPipeline::~RaytracingPipeline()
{
    pDescriptorSets.reset();
    pDescriptorSetLayout.reset();
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
