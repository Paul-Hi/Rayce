/// @file      descriptorSetLayout.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/device.hpp>

using namespace rayce;

DescriptorSetLayout::DescriptorSetLayout(const std::unique_ptr<Device>& logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings,
                                         const VkDescriptorSetLayoutCreateFlags createFlags, int32 variableBindingCount)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32>(layoutBindings.size());
    descriptorSetLayoutCreateInfo.flags        = createFlags;
    descriptorSetLayoutCreateInfo.pBindings    = layoutBindings.data();

    std::vector<VkDescriptorBindingFlags> extraBindingFlags;
    for (ptr_size i = 0; i < descriptorSetLayoutCreateInfo.bindingCount - 1; ++i)
    {
        extraBindingFlags.push_back(0);
    }
    extraBindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
    mVariableBindingCount = variableBindingCount;

    VkDescriptorSetLayoutBindingFlagsCreateInfo variableNumberInfoExtension{};
    variableNumberInfoExtension.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    variableNumberInfoExtension.bindingCount  = descriptorSetLayoutCreateInfo.bindingCount;
    variableNumberInfoExtension.pBindingFlags = extraBindingFlags.data();

    descriptorSetLayoutCreateInfo.pNext = variableBindingCount > 0 ? &variableNumberInfoExtension : nullptr;


    RAYCE_CHECK_VK(vkCreateDescriptorSetLayout(mVkLogicalDeviceRef, &descriptorSetLayoutCreateInfo, nullptr, &mVkDescriptorSetLayout), "Creating descriptor set layout failed!");
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (mVkDescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(mVkLogicalDeviceRef, mVkDescriptorSetLayout, nullptr);
    }
}
