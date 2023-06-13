/// @file      descriptorPool.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef DESCRIPTOR_POOL_HPP
#define DESCRIPTOR_POOL_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT DescriptorPool
    {
      public:
        DISABLE_COPY_MOVE_VK(DescriptorPool)

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
