/// @file      descriptorPool.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef DESCRIPTOR_POOL_HPP
#define DESCRIPTOR_POOL_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT DescriptorPool
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(DescriptorPool)

        DescriptorPool(const std::unique_ptr<class Device>& logicalDevice, std::vector<VkDescriptorPoolSize> poolSizes, uint32 maxSets, VkDescriptorPoolCreateFlags flags);
        ~DescriptorPool();

        VkDescriptorPool getVkDescriptorPool() const
        {
            return mVkDescriptorPool;
        }

    private:
        VkDescriptorPool mVkDescriptorPool;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // DESCRIPTOR_POOL_HPP
