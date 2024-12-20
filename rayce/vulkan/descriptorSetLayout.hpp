/// @file      descriptorSetLayout.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef DESCRIPTOR_LAYOUT_HPP
#define DESCRIPTOR_LAYOUT_HPP

namespace rayce
{
    class RAYCE_API_EXPORT DescriptorSetLayout
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(DescriptorSetLayout)

        DescriptorSetLayout(const std::unique_ptr<class Device>& logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, const VkDescriptorSetLayoutCreateFlags createFlags, int32 variableBindingCount = 0);
        ~DescriptorSetLayout();

        VkDescriptorSetLayout getVkDescriptorLayout() const
        {
            return mVkDescriptorSetLayout;
        }

        uint32 getVariableBindingCount() const
        {
            return mVariableBindingCount;
        }

    private:
        VkDescriptorSetLayout mVkDescriptorSetLayout;
        uint32 mVariableBindingCount;
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // DESCRIPTOR_LAYOUT_HPP
