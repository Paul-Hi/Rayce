/// @file      graphicsPipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT GraphicsPipeline
    {
      public:
        DISABLE_COPY_MOVE_VK(GraphicsPipeline)

        GraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, bool wireframe);
        ~GraphicsPipeline();

        VkRenderPass getVkRenderPass() const
        {
            return mVkRenderPass;
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
        VkRenderPass mVkRenderPass;
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkPipeline;
        VkDevice mVkLogicalDeviceRef;

        std::unique_ptr<class ShaderModule> pBaseVertexShader;
        std::unique_ptr<class ShaderModule> pBaseFragmentShader;
    };
} // namespace rayce

#endif // GRAPHICS_PIPELINE_HPP
