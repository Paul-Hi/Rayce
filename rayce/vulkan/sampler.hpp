/// @file      sampler.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef SAMPLER_HPP
#define SAMPLER_HPP

namespace rayce
{
    class RAYCE_API_EXPORT Sampler
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Sampler)

        Sampler(const std::unique_ptr<class Device>& logicalDevice, VkFilter magFilter, VkFilter minFilter,
                VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
                VkSamplerMipmapMode mipmapMode, bool anisotropy, bool compare, VkCompareOp compareOperation);

        /// @brief Destructor.
        ~Sampler();

        /// @brief Retrieves the vulkan sampler handle.
        /// @return The vulkan sampler handle.
        VkSampler getVkSampler() const
        {
            return mVkSampler;
        }

    private:
        /// @brief The vulkan Sampler handle.
        VkSampler mVkSampler;
        /// @brief The vulkan handle referencing the vulkan device.
        VkDevice mVkLogicalDeviceRef;
    };
} // namespace rayce

#endif // SAMPLER_HPP
