/// @file      descriptorSets.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/commandPool.hpp>
#include <vulkan/descriptorPool.hpp>
#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/descriptorSets.hpp>
#include <vulkan/device.hpp>

using namespace rayce;

DescriptorSets::DescriptorSets(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<DescriptorPool>& descriptorPool, const std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout,
                               const uint32 number)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mVkDescriptorPoolRef(descriptorPool->getVkDescriptorPool())
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(number, descriptorSetLayout->getVkDescriptorLayout());

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext              = nullptr;
    descriptorSetAllocateInfo.descriptorPool     = mVkDescriptorPoolRef;
    descriptorSetAllocateInfo.descriptorSetCount = number;
    descriptorSetAllocateInfo.pSetLayouts        = descriptorSetLayouts.data();

    mVkDescriptorSets.resize(number);
    RAYCE_CHECK_VK(vkAllocateDescriptorSets(mVkLogicalDeviceRef, &descriptorSetAllocateInfo, mVkDescriptorSets.data()), "Creating descriptor sets failed!");
}

DescriptorSets::~DescriptorSets()
{
    if (!mVkDescriptorSets.empty())
    {
        vkFreeDescriptorSets(mVkLogicalDeviceRef, mVkDescriptorPoolRef, static_cast<uint32>(mVkDescriptorSets.size()), mVkDescriptorSets.data());
        mVkDescriptorSets.clear();
    }
}

void DescriptorSets::update(const std::vector<VkWriteDescriptorSet>& writeDescriptorSets)
{
    vkUpdateDescriptorSets(mVkLogicalDeviceRef, static_cast<uint32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

