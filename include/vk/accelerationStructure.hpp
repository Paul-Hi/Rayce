/// @file      accelerationStructure.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    struct RAYCE_API_EXPORT AccelerationStructureInitData
    {
        VkAccelerationStructureTypeKHR type;
        VkDeviceAddress vertexDataDeviceAddress;
        VkDeviceAddress indexDataDeviceAddress;
        std::vector<VkDeviceAddress> blasDeviceAddresses;
        uint32 maxVertex;
        uint32 primitiveCount;
    };

    class RAYCE_API_EXPORT AccelerationStructure
    {
      public:
        RAYCE_DISABLE_COPY_MOVE(AccelerationStructure)

        AccelerationStructure(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, const AccelerationStructureInitData initData);
        ~AccelerationStructure();

        VkAccelerationStructureKHR getVkAccelerationStructure() const
        {
            return mVkAccelerationStructure;
        }

        VkDeviceAddress getDeviceAddress() const;

      private:
        VkAccelerationStructureKHR mVkAccelerationStructure;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class Buffer> pStorageBuffer;

        std::unique_ptr<class RTFunctions> pRTF;
    };
} // namespace rayce

#endif // ACCELERATION_STRUCTURE_HPP
