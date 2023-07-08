/// @file      descriptorSetLayout.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef DESCRIPTOR_LAYOUT_HPP
#define DESCRIPTOR_LAYOUT_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT DescriptorSetLayout
    {
      public:
        RAYCE_DISABLE_COPY_MOVE(DescriptorSetLayout)

        DescriptorSetLayout(const std::unique_ptr<class Device>& logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, const VkDescriptorSetLayoutCreateFlags createFlags);
        ~DescriptorSetLayout();

        VkDescriptorSetLayout getVkDescriptorLayout() const
        {
            return mVkDescriptorSetLayout;
        }

      private:
        VkDescriptorSetLayout mVkDescriptorSetLayout;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // DESCRIPTOR_LAYOUT_HPP