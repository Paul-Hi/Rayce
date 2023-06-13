/// @file      descriptorSets.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef DESCRIPTOR_SETS_HPP
#define DESCRIPTOR_SETS_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT DescriptorSets
    {
      public:
        DISABLE_COPY_MOVE_VK(DescriptorSets)

        DescriptorSets(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class DescriptorPool>& descriptorPool,
                       const std::unique_ptr<class DescriptorSetLayout>& descriptorSetLayout, const uint32 number);
        ~DescriptorSets();

        VkDescriptorSet& operator[](const uint32 i)
        {
            return mVkDescriptorSets[i];
        }

        void update(const std::vector<VkWriteDescriptorSet>& writeDescriptorSets);

      private:
        std::vector<VkDescriptorSet> mVkDescriptorSets;
        VkDevice mVkLogicalDeviceRef;
        VkDescriptorPool mVkDescriptorPoolRef;
    };
} // namespace rayce

#endif // DESCRIPTOR_SETS_HPP
