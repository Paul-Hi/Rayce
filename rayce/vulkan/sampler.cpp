/// @file      sampler.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/device.hpp>
#include <vulkan/sampler.hpp>

using namespace rayce;

Sampler::Sampler(const std::unique_ptr<Device>& logicalDevice, VkFilter magFilter, VkFilter minFilter,
                 VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
                 VkSamplerMipmapMode mipmapMode, bool anisotropy, bool compare, VkCompareOp compareOperation)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.flags                   = 0;
    samplerCreateInfo.magFilter               = magFilter;
    samplerCreateInfo.minFilter               = minFilter;
    samplerCreateInfo.mipmapMode              = mipmapMode;
    samplerCreateInfo.addressModeU            = addressModeU;
    samplerCreateInfo.addressModeV            = addressModeV;
    samplerCreateInfo.addressModeW            = addressModeW;
    samplerCreateInfo.mipLodBias              = 0.0f;
    samplerCreateInfo.anisotropyEnable        = anisotropy;
    samplerCreateInfo.maxAnisotropy           = anisotropy ? logicalDevice->getProperties().limits.maxSamplerAnisotropy : 1.0f;
    samplerCreateInfo.compareEnable           = compare;
    samplerCreateInfo.compareOp               = compareOperation;
    samplerCreateInfo.minLod                  = 0.0f;
    samplerCreateInfo.maxLod                  = 0.0f;
    samplerCreateInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    RAYCE_CHECK_VK(vkCreateSampler(mVkLogicalDeviceRef, &samplerCreateInfo, nullptr, &mVkSampler), "Creating sampler failed!");
}

Sampler::~Sampler()
{
    if (mVkSampler)
    {
        vkDestroySampler(mVkLogicalDeviceRef, mVkSampler, nullptr);
    }
}