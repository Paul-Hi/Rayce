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

        RaytracingPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, const std::unique_ptr<class Swapchain>& swapchain,
                           const std::unique_ptr<class AccelerationStructure>& tlas, const std::vector<VkBuffer>& vertexBuffers, const std::vector<VkBuffer>& indexBuffers,
                           CameraDataRT& cameraData, uint32 requiredImageDescriptors, const std::unique_ptr<class ImageView>& outputImage, uint32 framesInFlight);
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

        std::pair<VkDeviceAddress, uint32> getRayGenAddress() const
        {
            return { pShaderBindingTableBuffer->getDeviceAddress() + mRayGenOffset, mRayGenSize };
        }

        std::pair<VkDeviceAddress, uint32> getClosestHitAddress() const
        {
            return { pShaderBindingTableBuffer->getDeviceAddress() + mCHitOffset, mCHitSize };
        }

        std::pair<VkDeviceAddress, uint32> getMissAddress() const
        {
            return { pShaderBindingTableBuffer->getDeviceAddress() + mMissOffset, mMissSize };
        }

        uint32 getAlignedHandleSize() const
        {
            return mAlignedHandleSize;
        }

        void updateModelData(const std::unique_ptr<class Device>& logicalDevice, const std::vector<std::unique_ptr<struct InstanceData>>& instances, const std::vector<struct Sphere>& spheres,
                             const std::vector<std::unique_ptr<struct Material>>& materials, const std::vector<std::unique_ptr<struct Light>>& lights,
                             const std::vector<std::unique_ptr<class ImageView>>& images, const std::vector<std::unique_ptr<class Sampler>>& samplers);

        void updateCameraData(CameraDataRT& cameraData);

    private:
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutRT;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutCamera;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutModel;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutIndex;
        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayoutVertex;

        uint32 mFramesInFlight;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsRT;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsCamera;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsModel;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsIndex;
        std::unique_ptr<class DescriptorSets> pDescriptorSetsVertex;
        std::vector<std::unique_ptr<class Buffer>> mCameraBuffers;
        std::vector<void*> mCameraBuffersMapped;
        std::vector<std::unique_ptr<class Buffer>> mInstanceBuffers;
        std::vector<void*> mInstanceBuffersMapped;
        std::vector<std::unique_ptr<class Buffer>> mMaterialBuffers;
        std::vector<void*> mMaterialBuffersMapped;
        std::vector<std::unique_ptr<class Buffer>> mLightBuffers;
        std::vector<void*> mLightBuffersMapped;
        std::vector<std::unique_ptr<class Buffer>> mSphereBuffers;
        std::vector<void*> mSphereBuffersMapped;
        std::unique_ptr<class Image> pAccumulationImage;
        std::unique_ptr<class ImageView> pAccumulationImageView;

        std::unique_ptr<class ShaderModule> pRayGenShader;
        std::unique_ptr<class ShaderModule> pClosestHitShader;
        std::unique_ptr<class ShaderModule> pSphereIntersectionShader;
        std::unique_ptr<class ShaderModule> pClosestHitSphereShader;
        std::unique_ptr<class ShaderModule> pMissShader;

        std::unique_ptr<class Buffer> pShaderBindingTableBuffer;
        uint32 mAlignedHandleSize;
        uint32 mRayGenOffset;
        uint32 mCHitOffset;
        uint32 mMissOffset;
        uint32 mRayGenSize;
        uint32 mCHitSize;
        uint32 mMissSize;

        std::unique_ptr<class DescriptorPool> pDescriptorPool;

        std::unique_ptr<class RTFunctions> pRTF;
    };
} // namespace rayce

#endif // RAYTRACING_PIPELINE_HPP
