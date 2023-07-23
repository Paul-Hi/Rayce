/// @file      accelerationStructure.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/accelerationStructure.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/device.hpp>
#include <vulkan/immediateSubmit.hpp>
#include <vulkan/rtFunctions.hpp>
#include <vulkan/vertex.hpp>

using namespace rayce;

AccelerationStructure::AccelerationStructure(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, const AccelerationStructureInitData initData)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    pRTF = std::make_unique<RTFunctions>(logicalDevice);
    // bottom level
    if (initData.type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
    {
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        accelerationStructureGeometry.geometry.triangles.vertexData   = { initData.vertexDataDeviceAddress };
        accelerationStructureGeometry.geometry.triangles.maxVertex    = initData.maxVertex;
        accelerationStructureGeometry.geometry.triangles.vertexStride = Vertex::getSize();
        accelerationStructureGeometry.geometry.triangles.indexType    = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData    = { initData.indexDataDeviceAddress };

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        pRTF->vkGetAccelerationStructureBuildSizesKHR(mVkLogicalDeviceRef, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &initData.primitiveCount,
                                                      &accelerationStructureBuildSizesInfo);

        pStorageBuffer = std::make_unique<Buffer>(logicalDevice, accelerationStructureBuildSizesInfo.accelerationStructureSize,
                                                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        pStorageBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = pStorageBuffer->getVkBuffer();
        accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        RAYCE_CHECK_VK(pRTF->vkCreateAccelerationStructureKHR(mVkLogicalDeviceRef, &accelerationStructureCreateInfo, nullptr, &mVkAccelerationStructure), "Creating accelerating structure failed!");

        // Build

        // scratch buffer
        std::unique_ptr<Buffer> scratchBuffer =
            std::make_unique<Buffer>(logicalDevice, accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        scratchBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getDeviceAddress();

        accelerationStructureBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount             = 1;
        accelerationStructureBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
        accelerationStructureBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationStructureBuildGeometryInfo.dstAccelerationStructure  = mVkAccelerationStructure;
        accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount                                          = initData.primitiveCount;
        accelerationStructureBuildRangeInfo.primitiveOffset                                         = 0;
        accelerationStructureBuildRangeInfo.firstVertex                                             = 0;
        accelerationStructureBuildRangeInfo.transformOffset                                         = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        // FIXME Next: We get a VK_ERROR_DEVICE_LOST in here...
        ImmediateSubmit::Execute(logicalDevice, commandPool,
                                 [&](VkCommandBuffer commandBuffer)
                                 { pRTF->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos.data()); });

        scratchBuffer.reset();
    }
    else if (initData.type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
    {
        std::vector<VkAccelerationStructureInstanceKHR> accelerationStructureInstances;
        uint32 instance = 0; // gl_InstanceCustomIndexEXT
        for (const VkDeviceAddress& blasAddress : initData.blasDeviceAddresses)
        {
            VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
            accelerationStructureInstance.transform                              = initData.transformMatrices[instance];
            accelerationStructureInstance.instanceCustomIndex                    = instance++;
            accelerationStructureInstance.mask                                   = 0xFF;
            accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
            accelerationStructureInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            accelerationStructureInstance.accelerationStructureReference         = blasAddress;
            accelerationStructureInstances.push_back(accelerationStructureInstance);
        }

        std::unique_ptr<Buffer> instanceBuffer =
            std::make_unique<Buffer>(logicalDevice, sizeof(VkAccelerationStructureInstanceKHR),
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        instanceBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Buffer::uploadDataWithStagingBuffer(logicalDevice, commandPool, *instanceBuffer, accelerationStructureInstances);

        VkDeviceAddress instanceBufferDeviceAddress = instanceBuffer->getDeviceAddress();

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data            = { instanceBufferDeviceAddress };

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        pRTF->vkGetAccelerationStructureBuildSizesKHR(mVkLogicalDeviceRef, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &initData.primitiveCount,
                                                      &accelerationStructureBuildSizesInfo);

        pStorageBuffer = std::make_unique<Buffer>(logicalDevice, accelerationStructureBuildSizesInfo.accelerationStructureSize,
                                                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        pStorageBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = pStorageBuffer->getVkBuffer();
        accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        RAYCE_CHECK_VK(pRTF->vkCreateAccelerationStructureKHR(mVkLogicalDeviceRef, &accelerationStructureCreateInfo, nullptr, &mVkAccelerationStructure), "Creating accelerating structure failed!");

        // Build

        // scratch buffer
        std::unique_ptr<Buffer> scratchBuffer =
            std::make_unique<Buffer>(logicalDevice, accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        scratchBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getDeviceAddress();

        accelerationStructureBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount             = 1;
        accelerationStructureBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
        accelerationStructureBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationStructureBuildGeometryInfo.dstAccelerationStructure  = mVkAccelerationStructure;
        accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount                                          = initData.primitiveCount;
        accelerationStructureBuildRangeInfo.primitiveOffset                                         = 0;
        accelerationStructureBuildRangeInfo.firstVertex                                             = 0;
        accelerationStructureBuildRangeInfo.transformOffset                                         = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        ImmediateSubmit::Execute(logicalDevice, commandPool,
                                 [&](VkCommandBuffer commandBuffer)
                                 { pRTF->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos.data()); });

        scratchBuffer.reset();
    }
}

VkDeviceAddress AccelerationStructure::getDeviceAddress() const
{
    VkAccelerationStructureDeviceAddressInfoKHR info{};
    info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    info.pNext                 = nullptr;
    info.accelerationStructure = mVkAccelerationStructure;

    return pRTF->vkGetAccelerationStructureDeviceAddressKHR(mVkLogicalDeviceRef, &info);
}

AccelerationStructure::~AccelerationStructure()
{
    if (pStorageBuffer)
    {
        pStorageBuffer.reset();
    }
    if (mVkAccelerationStructure)
    {
        pRTF->vkDestroyAccelerationStructureKHR(mVkLogicalDeviceRef, mVkAccelerationStructure, nullptr);
    }
}
