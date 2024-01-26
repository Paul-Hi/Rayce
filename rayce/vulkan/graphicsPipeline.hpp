/// @file      graphicsPipeline.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

namespace rayce
{
    /// @brief A vulkan graphics pipeline.
    class RAYCE_API_EXPORT GraphicsPipeline
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(GraphicsPipeline)

        /// @brief Constructs a new @a GraphicsPipeline.
        /// @param[in] logicalDevice The logical @a Device.
        /// @param[in] swapchain The current @a Swapchain.
        /// @param[in] wireframe True if the pipeline should draw wireframe, else False.
        GraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, bool wireframe);

        /// @brief Destructor.
        ~GraphicsPipeline();

        /// @brief Returns the pipelines @a RenderPass.
        /// @return The pipelines @a RenderPass.
        const std::unique_ptr<class RenderPass>& getRenderPass() const
        {
            return pRenderPass;
        }

        /// @brief Returns the pipelines vulkan pipeline layout.
        /// @return The pipelines vulkan pipeline layout.
        VkPipelineLayout getVkPipelineLayout() const
        {
            return mVkPipelineLayout;
        }

        /// @brief Returns the pipelines vulkan pipeline.
        /// @return The pipelines vulkan pipeline.
        VkPipeline getVkPipeline() const
        {
            return mVkPipeline;
        }

    private:
        /// @brief Vulkan pipeline layout handle.
        VkPipelineLayout mVkPipelineLayout;
        /// @brief Vulkan pipeline handle.
        VkPipeline mVkPipeline;
        /// @brief Vulkan handle reference to the device.
        VkDevice mVkLogicalDeviceRef;

        /// @brief The pipelines @a RenderPass.
        std::unique_ptr<class RenderPass> pRenderPass;

        /// @brief The pipelines vertex @a ShaderModule.
        std::unique_ptr<class ShaderModule> pBaseVertexShader;
        /// @brief The pipelines fragment @a ShaderModule.
        std::unique_ptr<class ShaderModule> pBaseFragmentShader;
    };
} // namespace rayce

#endif // GRAPHICS_PIPELINE_HPP
