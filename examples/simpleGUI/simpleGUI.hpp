/// @file      simpleGUI.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#include "hostDeviceInterop.slang"
#include <app/rayceApp.hpp>

namespace rayce
{
    class SimpleGUI : public RayceApp
    {
    public:
        SimpleGUI(const RayceOptions& options);

        bool onInitialize() override;
        bool onShutdown() override;
        void onFrameDraw() override;
        void onUpdate(float dt) override;
        void onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex) override;
        void onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex) override;
        void recreateSwapchain() override;

    private:
        void recreateRTData();

        std::unique_ptr<class RaytracingPipeline> pRaytracingPipeline;
        std::unique_ptr<class RayceScene> pScene;
        std::unique_ptr<class Camera> pCamera;
        EIntegratorType mIntegratorType;
        int32 mAccumulationFrame;
        int32 mMaxDepth;
        int32 mMaxSamples;
        bool mViewportChange;
        bool mReInitialize;
        bool mRecreateRTData;
        uvec2 mViewportPanelSize;

        std::unique_ptr<class Sampler> pDefaultSampler;

        std::vector<struct Sphere> mSpheres;
        std::unique_ptr<class Buffer> mAABBBuffer;
        std::vector<std::unique_ptr<class AccelerationStructure>> mBLAS;
        std::unique_ptr<class AccelerationStructure> pTLAS;
        std::vector<VkBuffer> mVertexBuffers;
        std::vector<VkBuffer> mIndexBuffers;

        std::vector<std::unique_ptr<struct InstanceData>> mInstances;

        std::unique_ptr<class Image> pRaytracingTargetImage;
        std::unique_ptr<class ImageView> pRaytracingTargetView;

        VkDescriptorSet mImguiVkSet;
    };
} // namespace rayce
