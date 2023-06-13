/// @file      rtFunctions.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/rtFunctions.hpp>

using namespace rayce;

template <typename T>
T getProcAddress(const VkDevice& device, const char* const name)
{
    T func = reinterpret_cast<T>(vkGetDeviceProcAddr(device, name));

    RAYCE_CHECK_NOTNULL(func, "Loading %s failed!", name);

    return func;
}

RTFunctions::RTFunctions(const std::unique_ptr<class Device>& logicalDevice)
    : vkCreateAccelerationStructureKHR(getProcAddress<PFN_vkCreateAccelerationStructureKHR>(logicalDevice->getVkDevice(), "vkCreateAccelerationStructureKHR"))
    , vkDestroyAccelerationStructureKHR(getProcAddress<PFN_vkDestroyAccelerationStructureKHR>(logicalDevice->getVkDevice(), "vkDestroyAccelerationStructureKHR"))
    , vkGetAccelerationStructureBuildSizesKHR(getProcAddress<PFN_vkGetAccelerationStructureBuildSizesKHR>(logicalDevice->getVkDevice(), "vkGetAccelerationStructureBuildSizesKHR"))
    , vkCmdBuildAccelerationStructuresKHR(getProcAddress<PFN_vkCmdBuildAccelerationStructuresKHR>(logicalDevice->getVkDevice(), "vkCmdBuildAccelerationStructuresKHR"))
    , vkCmdCopyAccelerationStructureKHR(getProcAddress<PFN_vkCmdCopyAccelerationStructureKHR>(logicalDevice->getVkDevice(), "vkCmdCopyAccelerationStructureKHR"))
    , vkCmdTraceRaysKHR(getProcAddress<PFN_vkCmdTraceRaysKHR>(logicalDevice->getVkDevice(), "vkCmdTraceRaysKHR"))
    , vkCreateRayTracingPipelinesKHR(getProcAddress<PFN_vkCreateRayTracingPipelinesKHR>(logicalDevice->getVkDevice(), "vkCreateRayTracingPipelinesKHR"))
    , vkGetRayTracingShaderGroupHandlesKHR(getProcAddress<PFN_vkGetRayTracingShaderGroupHandlesKHR>(logicalDevice->getVkDevice(), "vkGetRayTracingShaderGroupHandlesKHR"))
    , vkGetAccelerationStructureDeviceAddressKHR(getProcAddress<PFN_vkGetAccelerationStructureDeviceAddressKHR>(logicalDevice->getVkDevice(), "vkGetAccelerationStructureDeviceAddressKHR"))
    , vkCmdWriteAccelerationStructuresPropertiesKHR(getProcAddress<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(logicalDevice->getVkDevice(), "vkCmdWriteAccelerationStructuresPropertiesKHR"))
{
}

RTFunctions::~RTFunctions() {}
