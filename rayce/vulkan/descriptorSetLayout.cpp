/// @file      descriptorSetLayout.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/device.hpp>

using namespace rayce;

DescriptorSetLayout::DescriptorSetLayout(const std::unique_ptr<Device>& logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings,
                                         const VkDescriptorSetLayoutCreateFlags createFlags)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32>(layoutBindings.size());
    descriptorSetLayoutCreateInfo.flags        = createFlags;
    descriptorSetLayoutCreateInfo.pBindings    = layoutBindings.data();
    RAYCE_CHECK_VK(vkCreateDescriptorSetLayout(mVkLogicalDeviceRef, &descriptorSetLayoutCreateInfo, nullptr, &mVkDescriptorSetLayout), "Creating descriptor set layout failed!");
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (mVkDescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(mVkLogicalDeviceRef, mVkDescriptorSetLayout, nullptr);
    }
}
