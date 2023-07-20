/// @file      raytracingPipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYTRACING_PIPELINE_HPP
#define RAYTRACING_PIPELINE_HPP

namespace rayce
{
    class RAYCE_API_EXPORT RaytracingPipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(RaytracingPipeline)

        RaytracingPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const std::unique_ptr<class AccelerationStructure>& tlas, const std::unique_ptr<class ImageView>& outputImage,
                           uint32 framesInFlight);
        ~RaytracingPipeline();

        VkPipelineLayout getVkPipelineLayout() const
        {
            return mVkPipelineLayout;
        }

        VkPipeline getVkPipeline() const
        {
            return mVkPipeline;
        }

        std::vector<VkDescriptorSet> getVkDescriptorSets(uint32 idx) const;

        const std::unique_ptr<class Buffer>& getShaderBindingTableBuffer() const
        {
            return pShaderBindingTableBuffer;
        }

        VkDeviceAddress getRayGenAddress() const
        {
            return pShaderBindingTableBuffer->getDeviceAddress() + mRayGenOffset;
        }

        VkDeviceAddress getClosestHitAddress() const
        {
            return pShaderBindingTableBuffer->getDeviceAddress() + mCHitOffset;
        }

        VkDeviceAddress getMissAddress() const
        {
            return pShaderBindingTableBuffer->getDeviceAddress() + mMissOffset;
        }

        uint32 getAlignedHandleSize() const
        {
            return mAlignedHandleSize;
        }

        void updateImageView(const std::unique_ptr<class ImageView>& image);

    private:
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutRT;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutCamera;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutModel;

        uint32 mFramesInFlight;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsRT;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsCamera;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsModel;
        std::vector<std::unique_ptr<class Buffer>> mCameraBuffers;
        std::vector<void*> mCameraBuffersMapped;

        std::unique_ptr<class ShaderModule> pRayGenShader;
        std::unique_ptr<class ShaderModule> pClosestHitShader;
        std::unique_ptr<class ShaderModule> pMissShader;

        std::unique_ptr<class Buffer> pShaderBindingTableBuffer;
        uint32 mAlignedHandleSize;
        uint32 mRayGenOffset;
        uint32 mCHitOffset;
        uint32 mMissOffset;

        std::unique_ptr<class DescriptorPool> pDescriptorPool;

        std::unique_ptr<class Sampler> pTextureSampler;

        std::unique_ptr<class RTFunctions> pRTF;

        struct CameraBufferRT
        {
            mat4 inverseView;
            mat4 inverseProjection;
        };
    };
} // namespace rayce

#endif // RAYTRACING_PIPELINE_HPP
