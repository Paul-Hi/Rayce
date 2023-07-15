/// @file      raytracingPipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef RAYTRACING_PIPELINE_HPP
#define RAYTRACING_PIPELINE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT RaytracingPipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(RaytracingPipeline)

        RaytracingPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class AccelerationStructure>& tlas, const std::unique_ptr<class ImageView>& outputImage,
                           uint32 descriptorsInFlight);
        ~RaytracingPipeline();

        const std::unique_ptr<class DescriptorSetLayout>& getVkDescriptorSetLayout() const
        {
            return pDescriptorSetLayout;
        }

        VkPipelineLayout getVkPipelineLayout() const
        {
            return mVkPipelineLayout;
        }

        VkPipeline getVkPipeline() const
        {
            return mVkPipeline;
        }

        VkDescriptorSet getVkDescriptorSet(uint32 idx) const;

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

    private:
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class DescriptorSetLayout> pDescriptorSetLayout;

        uint32 mDescriptorsInFlight;
        std::unique_ptr<class DescriptorSets> pDescriptorSets;

        std::unique_ptr<class ShaderModule> pRayGenShader;
        std::unique_ptr<class ShaderModule> pClosestHitShader;
        std::unique_ptr<class ShaderModule> pMissShader;

        std::unique_ptr<class Buffer> pShaderBindingTableBuffer;
        uint32 mAlignedHandleSize;
        uint32 mRayGenOffset;
        uint32 mCHitOffset;
        uint32 mMissOffset;

        std::unique_ptr<class DescriptorPool> pDescriptorPool;

        std::unique_ptr<class RTFunctions> pRTF;
    };
} // namespace rayce

#endif // RAYTRACING_PIPELINE_HPP
