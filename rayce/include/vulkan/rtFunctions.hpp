/// @file      rtFunctions.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef RT_FUNCTIONS_HPP
#define RT_FUNCTIONS_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT RTFunctions
    {
      public:
        DISABLE_COPY_MOVE_VK(RTFunctions)

        RTFunctions(const std::unique_ptr<class Device>& logicalDevice);
        ~RTFunctions();

        const std::function<VkResult(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                     VkAccelerationStructureKHR* pAccelerationStructure)>
            vkCreateAccelerationStructureKHR;

        const std::function<void(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks* pAllocator)> vkDestroyAccelerationStructureKHR;

        const std::function<void(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const uint32* pMaxPrimitiveCounts,
                                 VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)>
            vkGetAccelerationStructureBuildSizesKHR;

        const std::function<void(VkCommandBuffer commandBuffer, uint32 infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
                                 const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)>
            vkCmdBuildAccelerationStructuresKHR;

        const std::function<void(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR* pInfo)> vkCmdCopyAccelerationStructureKHR;

        const std::function<void(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
                                 const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32 width, uint32 height,
                                 uint32 depth)>
            vkCmdTraceRaysKHR;

        const std::function<VkResult(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32 createInfoCount,
                                     const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)>
            vkCreateRayTracingPipelinesKHR;

        const std::function<VkResult(VkDevice device, VkPipeline pipeline, uint32 firstGroup, uint32 groupCount, size_t dataSize, void* pData)> vkGetRayTracingShaderGroupHandlesKHR;

        const std::function<VkDeviceAddress(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)> vkGetAccelerationStructureDeviceAddressKHR;

        const std::function<void(VkCommandBuffer commandBuffer, uint32 accelerationStructureCount, const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType,
                                 VkQueryPool queryPool, uint32 firstQuery)>
            vkCmdWriteAccelerationStructuresPropertiesKHR;
    };
} // namespace rayce

#endif // RT_FUNCTIONS_HPP
