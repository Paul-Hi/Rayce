/// @file      accelerationStructure.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

namespace rayce
{
    /// @brief Initialization data for @a AccelerationStructures.
    struct RAYCE_API_EXPORT AccelerationStructureInitData
    {
        /// @brief The structure type (BLAS or TLAS).
        VkAccelerationStructureTypeKHR type;
        /// @brief Device address of the vertex buffer (Only BLAS).
        VkDeviceAddress vertexDataDeviceAddress;
        /// @brief Device address of the index buffer (Only BLAS).
        VkDeviceAddress indexDataDeviceAddress;
        /// @brief List of device addresses of bottom level acceleration structures (Only TLAS).
        std::vector<VkDeviceAddress> blasDeviceAddresses;
        /// @brief List of transform matrices of bottom level acceleration structures (Only TLAS).
        std::vector<VkTransformMatrixKHR> transformMatrices;
        /// @brief Maximum index of any vertex (Only BLAS).
        uint32 maxVertex;
        /// @brief Number of primitives.
        uint32 primitiveCount;
    };

    /// @brief Any kind of acceleration structure.
    class RAYCE_API_EXPORT AccelerationStructure
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(AccelerationStructure)

        /// @brief Constructs a new @a AccelerationStructure.
        /// @param[in] logicalDevice The logical @a Device.
        /// @param[in] commandPool The @a CommandPool to use.
        /// @param[in] initData The @a AccelerationStructureInitData providing initialization information.
        AccelerationStructure(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, const AccelerationStructureInitData initData);

        /// @brief Destructor.
        ~AccelerationStructure();

        /// @brief Retrieves the underlying vulkan acceleration structure handle.
        /// @return The underlying vulkan acceleration structure handle.
        VkAccelerationStructureKHR getVkAccelerationStructure() const
        {
            return mVkAccelerationStructure;
        }

        /// @brief Retrieves the vulkan device address of the @a AccelerationStructure.
        /// @return The vulkan device address of the @a AccelerationStructure.
        VkDeviceAddress getDeviceAddress() const;

    private:
        /// @brief The underlying vulkan acceleration structure handle.
        VkAccelerationStructureKHR mVkAccelerationStructure;
        /// @brief The vulkan device handle.
        VkDevice mVkLogicalDeviceRef;

        /// @brief The storage @a Buffer storing the @a AccelerationStructures data.
        std::unique_ptr<class Buffer> pStorageBuffer;

        /// @brief Loaded raytracing extension functions are stored there.
        std::unique_ptr<class RTFunctions> pRTF;
    };
} // namespace rayce

#endif // ACCELERATION_STRUCTURE_HPP
