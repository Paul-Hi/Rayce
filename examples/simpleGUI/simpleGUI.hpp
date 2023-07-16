/// @file      simpleGUI.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

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
        void onUpdate() override;
        void onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex) override;
        void onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex) override;
        void recreateSwapchain() override;

      private:
        std::unique_ptr<class RaytracingPipeline> pRaytracingPipeline;
        std::unique_ptr<class RayceScene> pScene;

        std::vector<std::unique_ptr<class AccelerationStructure>> mBLAS;
        std::unique_ptr<class AccelerationStructure> pTLAS;

        std::unique_ptr<class Image> pRaytracingTargetImage;
        std::unique_ptr<class ImageView> pRaytracingTargetView;
    };
} // namespace rayce
