/// @file      pipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

/*
#pragma once

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT GraphicsPipelineSettings
    {
        std::vector<std::unique_ptr<class ShaderModule>> shaders = {};

        VkViewport viewport;
        VkRect2D scissorRectangle;

        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode             = VK_POLYGON_MODE_FILL;
        float lineWidth                       = 1.0;
        VkCullModeFlagBits cullMode           = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace                 = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkClearValue clearColor        = { 0.0, 0.0, 0.0, 0.0 };
        VkClearValue clearDepthStencil = { 0.0, 0.0, 0.0, 0.0 };

        std::vector<std::shared_ptr<class Buffer>> vertexBuffers = {};
        std::shared_ptr<class Buffer> indexBuffer                = {};
        VkIndexType indexType                                    = VK_INDEX_TYPE_UINT32;

        std::vector<VkBool32> attachmentBlending                          = {};
        std::vector<std::shared_ptr<class Texture2D>> colorOutputTextures = {};
        std::shared_ptr<class Texture2D> depthStencilOutputTexture        = {};

        VkBool32 depthTest       = VK_TRUE;
        VkBool32 depthWrite      = VK_TRUE;
        VkCompareOp depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
        // VkBool32 stencilTest;
    };

    struct RAYCE_API_EXPORT ComputePipelineSettings
    {
        std::unique_ptr<class ShaderModule> shader = nullptr;
    };

    struct RAYCE_API_EXPORT RTPipelineSettings
    {
        std::vector<std::unique_ptr<class ShaderModule>> shaders = {};
    };

    enum class RAYCE_API_EXPORT EPipelineType : byte
    {
        Graphics,
        Compute,
        Raytracing
    };

    /// @brief A vulkan pipeline.
    class RAYCE_API_EXPORT Pipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Pipeline)

        static std::shared_ptr<Pipeline> createGraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const GraphicsPipelineSettings& settings);
        static std::shared_ptr<Pipeline> createComputePipeline(const std::unique_ptr<class Device>& logicalDevice, const ComputePipelineSettings& settings);
        static std::shared_ptr<Pipeline> createRTPipeline(const std::unique_ptr<class Device>& logicalDevice, const RTPipelineSettings& settings);

        Pipeline(const std::unique_ptr<class Device>& logicalDevice, const GraphicsPipelineSettings& settings);
        Pipeline(const std::unique_ptr<class Device>& logicalDevice, const ComputePipelineSettings& settings);
        Pipeline(const std::unique_ptr<class Device>& logicalDevice, const RTPipelineSettings& settings);

        ~Pipeline();

    private:
        VkDevice mVkLogicalDeviceRef;

        EPipelineType mPipelineType;
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;

        std::vector<std::unique_ptr<class DescriptorSetLayout>> mDescriptorSetLayouts;
        std::vector<VkPushConstantRange> mPushConstantRanges;

        // std::unique_ptr<class RTFunctions> pRTF = nullptr;

    private:
        void mergePushConstantRanges();
    };
} // namespace rayce

#endif // PIPELINE_HPP
*/