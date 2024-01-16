/// @file      pipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

class GraphicsPipelineSettings;
class ComputePipelineSettings;
class RTPipelineSettings;

enum class EPipelineType : uint8
{
    Graphics,
    Compute,
    Raytracing
};

namespace rayce
{
    /// @brief A vulkan pipeline.
    class RAYCE_API_EXPORT Pipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Pipeline)

        static std::shared_ptr<Pipeline> createGraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const GraphicsPipelineSettings& settings);
        static std::shared_ptr<Pipeline> createComputePipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const ComputePipelineSettings& settings);
        static std::shared_ptr<Pipeline> createRTPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const RTPipelineSettings& settings);

        ~Pipeline() = default;

    private:
        VkDevice mVkLogicalDeviceRef;

        EPipelineType mPipelineType;
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;

        std::vector<class DescriptorSetLayout> mDescriptorSetLayouts;

        std::unique_ptr<class RTFunctions> pRTF = nullptr;

    private:
        Pipeline() = default;
    };
} // namespace rayce

#endif // PIPELINE_HPP
