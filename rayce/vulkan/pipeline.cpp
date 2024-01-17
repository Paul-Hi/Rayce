/// @file      pipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "pipeline.hpp"
#include "device.hpp"
#include "swapchain.hpp"

using namespace rayce;

std::shared_ptr<Pipeline> Pipeline::createGraphicsPipeline(const std::unique_ptr<Device>& logicalDevice, const GraphicsPipelineSettings& settings)
{
    return std::make_shared<Pipeline>(logicalDevice, settings);
}

std::shared_ptr<Pipeline> Pipeline::createComputePipeline(const std::unique_ptr<Device>& logicalDevice, const ComputePipelineSettings& settings)
{
    return std::make_shared<Pipeline>(logicalDevice, settings);
}

std::shared_ptr<Pipeline> Pipeline::createRTPipeline(const std::unique_ptr<Device>& logicalDevice, const RTPipelineSettings& settings)
{
    return std::make_shared<Pipeline>(logicalDevice, settings);
}

Pipeline::Pipeline(const std::unique_ptr<Device>& logicalDevice, const GraphicsPipelineSettings& settings)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mPipelineType(EPipelineType::Graphics)
{
    RAYCE_ASSERT(settings.colorOutputTextures.size() > 0, "No color outputs for graphics pipeline.");

    // Dynamic State
    // We define a static viewport and scissor for now
    // std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 0;       // static_cast<uint32>(dynamicStates.size());
    dynamicState.pDynamicStates    = nullptr; // dynamicStates.data();

    uint32 vertexShaderIdx = 0;
    for (uint32 i = 0; i < settings.shaders.size(); ++i)
    {
        if (settings.shaders[i]->mStage == VK_SHADER_STAGE_VERTEX_BIT)
        {
            vertexShaderIdx = i;
            break;
        }
    }

    const std::shared_ptr<Shader>& vertexShader = settings.shaders[vertexShaderIdx];
    RAYCE_ASSERT(vertexShader->mReflected, "Vertex shader not reflected for graphics pipeline creation.");

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    bindingDescriptions.reserve(vertexShader->mVertexInputs.size());
    attributeDescriptions.reserve(vertexShader->mVertexInputs.size());
    for (auto& [bindingDesc, attribDesc] : vertexShader->mVertexInputs)
    {
        bindingDescriptions.push_back(bindingDesc);
        attributeDescriptions.push_back(attribDesc);
    }

    // Vertex Input State
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = static_cast<uint32>(bindingDescriptions.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = bindingDescriptions.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32>(attributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = settings.primitiveTopology;
    inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    // Viewport State
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports    = &settings.viewport;
    viewportStateCreateInfo.scissorCount  = 1;
    viewportStateCreateInfo.pScissors     = &settings.scissorRectangle;

    // Rasterization State
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode             = settings.polygonMode;
    rasterizationStateCreateInfo.lineWidth               = settings.lineWidth;
    rasterizationStateCreateInfo.cullMode                = settings.cullMode;
    rasterizationStateCreateInfo.frontFace               = settings.frontFace;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;

    // Multisample State
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable  = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = settings.rasterizationSamples;

    // Blend Attachment States (when blending ops are hardcoded)
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
    VkPipelineColorBlendAttachmentState colorBlendAttachmentStateTemplate{};
    colorBlendAttachmentStateTemplate.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentStateTemplate.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentStateTemplate.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentStateTemplate.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachmentStateTemplate.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentStateTemplate.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentStateTemplate.alphaBlendOp        = VK_BLEND_OP_ADD;
    if (settings.attachmentBlending.empty())
    {
        colorBlendAttachmentStateTemplate.blendEnable = VK_FALSE;
        blendAttachmentStates                         = std::vector<VkPipelineColorBlendAttachmentState>(settings.colorOutputTextures.size(), colorBlendAttachmentStateTemplate);
    }
    else
    {
        for (const VkBool32& blendingEnabled : settings.attachmentBlending)
        {
            colorBlendAttachmentStateTemplate.blendEnable = blendingEnabled;
            blendAttachmentStates.push_back(colorBlendAttachmentStateTemplate);
        }
    }

    // Depth Stencil State
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable  = settings.depthTest;
    depthStencilStateCreateInfo.depthWriteEnable = settings.depthWrite;
    depthStencilStateCreateInfo.depthCompareOp   = settings.depthCompare;

    // Color Blend State
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable     = VK_FALSE;
    colorBlendStateCreateInfo.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount   = static_cast<uint32>(blendAttachmentStates.size());
    colorBlendStateCreateInfo.pAttachments      = blendAttachmentStates.data();
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;


    // create binding layouts and create descriptor sets with shader reflection
}

Pipeline::Pipeline(const std::unique_ptr<Device>& logicalDevice, const ComputePipelineSettings& settings)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
}

Pipeline::Pipeline(const std::unique_ptr<Device>& logicalDevice, const RTPipelineSettings& settings)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
}
