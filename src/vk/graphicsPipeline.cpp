/// @file      graphicsPipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vk/device.hpp>
#include <vk/graphicsPipeline.hpp>
#include <vk/renderPass.hpp>
#include <vk/shaderModule.hpp>
#include <vk/swapchain.hpp>
#include <vk/vertex.hpp>

using namespace rayce;

GraphicsPipeline::GraphicsPipeline(const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<Swapchain>& swapchain, bool wireframe)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    // Dynamic state
    // We define a static viewport and scissor for now
    // std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 0;       // static_cast<uint32>(dynamicStates.size());
    dynamicState.pDynamicStates    = nullptr; // dynamicStates.data();

    // vertex input state - will be provided by assets
    VkVertexInputBindingDescription bindingDescription                    = Vertex::getVertexInputBindingDescription();
    std::array<VkVertexInputAttributeDescription, 1> attributeDescription = Vertex::getVertexInputAttributeDescription();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32>(attributeDescription.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = attributeDescription.data();

    // input assembly - assuming triangles for now
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    // viewport and scissor specification
    VkExtent2D swapchainExtent = swapchain->getSwapExtent();

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchainExtent.width);
    viewport.height   = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports    = &viewport;
    viewportStateCreateInfo.scissorCount  = 1;
    viewportStateCreateInfo.pScissors     = &scissor;

    // rasterization state setup
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode             = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp          = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor    = 0.0f;

    // multisample state setup - for now disabled
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable   = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading      = 1.0f;
    multisampleStateCreateInfo.pSampleMask           = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable      = VK_FALSE;

    // depth stencil setup
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable       = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.minDepthBounds        = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds        = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.front                 = {};
    depthStencilStateCreateInfo.back                  = {};

    // blend state per attachment - for now only one...
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable         = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;

    // blend state general
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable     = VK_FALSE;
    colorBlendStateCreateInfo.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount   = 1;
    colorBlendStateCreateInfo.pAttachments      = &colorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    // pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = 0;
    pipelineLayoutCreateInfo.pSetLayouts            = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;

    RAYCE_CHECK_VK(vkCreatePipelineLayout(mVkLogicalDeviceRef, &pipelineLayoutCreateInfo, nullptr, &mVkPipelineLayout), "Creating pipeline layout failed!");

    // render pass
    pRenderPass = std::make_unique<RenderPass>(logicalDevice, swapchain, VK_ATTACHMENT_LOAD_OP_CLEAR);

    // shader stages
    pBaseVertexShader   = std::make_unique<ShaderModule>(logicalDevice, ".\\assets\\shaders\\basic.vert.spv");
    pBaseFragmentShader = std::make_unique<ShaderModule>(logicalDevice, ".\\assets\\shaders\\basic.frag.spv");

    VkPipelineShaderStageCreateInfo shaderStages[] = { pBaseVertexShader->createShaderStage(VK_SHADER_STAGE_VERTEX_BIT), pBaseFragmentShader->createShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT) };

    // finally creating the pipeline :D
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount          = 2;
    pipelineCreateInfo.pStages             = shaderStages;
    pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.layout              = mVkPipelineLayout;
    pipelineCreateInfo.renderPass          = pRenderPass->getVkRenderPass();
    pipelineCreateInfo.subpass             = 0;
    pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex   = -1;

    RAYCE_CHECK_VK(vkCreateGraphicsPipelines(mVkLogicalDeviceRef, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mVkPipeline), "Creating graphics pipeline failed!");

    RAYCE_LOG_INFO("Created graphics pipeline!");
}

GraphicsPipeline::~GraphicsPipeline()
{
    if (mVkPipeline)
    {
        vkDestroyPipeline(mVkLogicalDeviceRef, mVkPipeline, nullptr);
    }
    if (mVkPipelineLayout)
    {
        vkDestroyPipelineLayout(mVkLogicalDeviceRef, mVkPipelineLayout, nullptr);
    }
}
