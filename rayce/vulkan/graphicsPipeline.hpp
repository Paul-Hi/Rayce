/// @file      graphicsPipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT GraphicsPipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(GraphicsPipeline)

        GraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, bool wireframe);
        ~GraphicsPipeline();

        const std::unique_ptr<class RenderPass>& getRenderPass() const
        {
            return pRenderPass;
        }

        VkPipelineLayout getVkPipelineLayout() const
        {
            return mVkPipelineLayout;
        }

        VkPipeline getVkPipeline() const
        {
            return mVkPipeline;
        }

    private:
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class RenderPass> pRenderPass;

        std::unique_ptr<class ShaderModule> pBaseVertexShader;
        std::unique_ptr<class ShaderModule> pBaseFragmentShader;
    };
} // namespace rayce

#endif // GRAPHICS_PIPELINE_HPP
