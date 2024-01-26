/// @file      pipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/descriptorSetLayout.hpp>
#include <vulkan/descriptorSets.hpp>
#include <vulkan/device.hpp>
#include <vulkan/pipeline.hpp>
#include <vulkan/swapchain.hpp>

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

    int32 vertexShaderIdx = -1;
    for (uint32 i = 0; i < settings.shaders.size(); ++i)
    {
        if (settings.shaders[i]->mStage == VK_SHADER_STAGE_VERTEX_BIT)
        {
            vertexShaderIdx = i;
            break;
        }
    }
    RAYCE_ASSERT(vertexShaderIdx >= 0, "No vertex shader in graphics pipeline.");

    const std::shared_ptr<Shader>& vertexShader = settings.shaders[vertexShaderIdx];
    RAYCE_ASSERT(vertexShader->mReflected, "Vertex shader not reflected for graphics pipeline creation.");

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.reserve(vertexShader->mVertexInputAttributes.size());
    for (auto& attribDesc : vertexShader->mVertexInputAttributes)
    {
        attributeDescriptions.push_back(attribDesc);
    }

    // Vertex Input State
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = &(vertexShader->mVertexInputBinding);
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

    // create shader stage create infos, binding layouts and descriptor sets
    std::vector<VkPipelineShaderStageCreateInfo> vkShaderStageCreateInfos;
    std::unordered_map<uint32, std::vector<VkDescriptorSetLayoutBinding>> setBindings;
    for (auto& shader : settings.shaders)
    {
        if (!shader->compiled())
        {
            RAYCE_LOG_ERROR("Shader not compiled!");
        }

        VkPipelineShaderStageCreateInfo createInfo{};
        createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        createInfo.stage  = shader->mStage;
        createInfo.module = shader->createShaderModule(logicalDevice);
        createInfo.pName  = shader->mShaderSpecialization.entryPoint.c_str();

        vkShaderStageCreateInfos.push_back(createInfo);

        for (auto& [set, descriptorSetLayoutBinding] : shader->mDescriptorSetLayoutBindings)
        {
            setBindings[set].push_back(descriptorSetLayoutBinding);
        }

        mPushConstantRanges.insert(mPushConstantRanges.end(), shader->mPushConstantRanges.begin(), shader->mPushConstantRanges.end());
    }

    mergePushConstantRanges();

    // create set layouts
    mDescriptorSetLayouts.resize(setBindings.size());
    std::vector<VkDescriptorSetLayout> vkLayouts(mDescriptorSetLayouts.size());
    for (auto& [set, bindings] : setBindings)
    {
        mDescriptorSetLayouts[set] = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);
        vkLayouts[set]             = mDescriptorSetLayouts[set]->getVkDescriptorLayout();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = static_cast<uint32>(vkLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts            = vkLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(mPushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges    = mPushConstantRanges.data();

    // create pipeline layout
    RAYCE_CHECK_VK(vkCreatePipelineLayout(mVkLogicalDeviceRef, &pipelineLayoutCreateInfo, nullptr, &mVkPipelineLayout), "Creating pipeline layout failed!");

    // create pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount          = static_cast<uint32>(vkShaderStageCreateInfos.size());
    pipelineCreateInfo.pStages             = vkShaderStageCreateInfos.data();
    pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.layout              = mVkPipelineLayout;
    pipelineCreateInfo.renderPass          = nullptr;
    pipelineCreateInfo.subpass             = 0;
    pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex   = -1;

    RAYCE_CHECK_VK(vkCreateGraphicsPipelines(mVkLogicalDeviceRef, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mVkPipeline), "Creating graphics pipeline failed!");

    // cleanup shader modules
    for (auto& stageCreateInfo : vkShaderStageCreateInfos)
    {
        vkDestroyShaderModule(mVkLogicalDeviceRef, stageCreateInfo.module, nullptr);
    }

    RAYCE_LOG_INFO("Created graphics pipeline!");
}

Pipeline::Pipeline(const std::unique_ptr<Device>& logicalDevice, const ComputePipelineSettings& settings)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mPipelineType(EPipelineType::Compute)
{
    RAYCE_ASSERT(settings.shader->mStage == VK_SHADER_STAGE_COMPUTE_BIT, "No compute shader in compute pipeline.");
    // create shader stage create infos, binding layouts and descriptor sets
    std::unordered_map<uint32, std::vector<VkDescriptorSetLayoutBinding>> setBindings;
    if (!settings.shader->compiled())
    {
        RAYCE_LOG_ERROR("Shader not compiled!");
    }

    VkPipelineShaderStageCreateInfo vkShaderStageCreateInfo{};
    vkShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkShaderStageCreateInfo.stage  = settings.shader->mStage;
    vkShaderStageCreateInfo.module = settings.shader->createShaderModule(logicalDevice);
    vkShaderStageCreateInfo.pName  = settings.shader->mShaderSpecialization.entryPoint.c_str();

    for (auto& [set, descriptorSetLayoutBinding] : settings.shader->mDescriptorSetLayoutBindings)
    {
        setBindings[set].push_back(descriptorSetLayoutBinding);
    }

    mPushConstantRanges.insert(mPushConstantRanges.end(), settings.shader->mPushConstantRanges.begin(), settings.shader->mPushConstantRanges.end());

    mergePushConstantRanges();

    // create set layouts
    mDescriptorSetLayouts.resize(setBindings.size());
    std::vector<VkDescriptorSetLayout> vkLayouts(mDescriptorSetLayouts.size());
    for (auto& [set, bindings] : setBindings)
    {
        mDescriptorSetLayouts[set] = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);
        vkLayouts[set]             = mDescriptorSetLayouts[set]->getVkDescriptorLayout();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = static_cast<uint32>(vkLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts            = vkLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(mPushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges    = mPushConstantRanges.data();

    // create pipeline layout
    RAYCE_CHECK_VK(vkCreatePipelineLayout(mVkLogicalDeviceRef, &pipelineLayoutCreateInfo, nullptr, &mVkPipelineLayout), "Creating pipeline layout failed!");

    // create pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage              = vkShaderStageCreateInfo;
    pipelineCreateInfo.layout             = mVkPipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex  = -1;

    RAYCE_CHECK_VK(vkCreateComputePipelines(mVkLogicalDeviceRef, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mVkPipeline), "Creating compute pipeline failed!");

    // cleanup shader module
    vkDestroyShaderModule(mVkLogicalDeviceRef, vkShaderStageCreateInfo.module, nullptr);

    RAYCE_LOG_INFO("Created compute pipeline!");
}

Pipeline::Pipeline(const std::unique_ptr<Device>& logicalDevice, const RTPipelineSettings& settings)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mPipelineType(EPipelineType::Raytracing)
{
    // create shader stage create infos, binding layouts and descriptor sets
    std::vector<VkPipelineShaderStageCreateInfo> vkShaderStageCreateInfos;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> vkRTShaderGroupCreateInfos;
    std::unordered_map<uint32, std::vector<VkDescriptorSetLayoutBinding>> setBindings;
    uint32 stageIdx = 0;
    for (auto& shader : settings.shaders)
    {
        if (!shader->compiled())
        {
            RAYCE_LOG_ERROR("Shader not compiled!");
        }

        VkPipelineShaderStageCreateInfo createInfo{};
        createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        createInfo.stage  = shader->mStage;
        createInfo.module = shader->createShaderModule(logicalDevice);
        createInfo.pName  = shader->mShaderSpecialization.entryPoint.c_str();

        vkShaderStageCreateInfos.push_back(createInfo);

        VkRayTracingShaderGroupCreateInfoKHR groupCreateInfo{};
        groupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        groupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
        groupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
        groupCreateInfo.generalShader      = VK_SHADER_UNUSED_KHR;
        groupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        switch (shader->mStage)
        {
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
        case VK_SHADER_STAGE_MISS_BIT_KHR:
        {
            groupCreateInfo.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            groupCreateInfo.generalShader = stageIdx;
            break;
        }
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        {
            groupCreateInfo.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            groupCreateInfo.closestHitShader = stageIdx;
            break;
        }
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
        {
            groupCreateInfo.type         = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            groupCreateInfo.anyHitShader = stageIdx;
            break;
        }
        case VK_SHADER_STAGE_VERTEX_BIT:
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        case VK_SHADER_STAGE_GEOMETRY_BIT:
        case VK_SHADER_STAGE_FRAGMENT_BIT:
        case VK_SHADER_STAGE_COMPUTE_BIT:
        case VK_SHADER_STAGE_ALL_GRAPHICS:
        case VK_SHADER_STAGE_ALL:
        case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
        case VK_SHADER_STAGE_TASK_BIT_NV:
        case VK_SHADER_STAGE_MESH_BIT_NV:
        case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI:
        case VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI:
            break;
        }

        vkRTShaderGroupCreateInfos.push_back(groupCreateInfo);

        for (auto& [set, descriptorSetLayoutBinding] : shader->mDescriptorSetLayoutBindings)
        {
            setBindings[set].push_back(descriptorSetLayoutBinding);
        }

        mPushConstantRanges.insert(mPushConstantRanges.end(), shader->mPushConstantRanges.begin(), shader->mPushConstantRanges.end());

        mergePushConstantRanges();

        stageIdx++;
    }

    // create set layouts
    mDescriptorSetLayouts.resize(setBindings.size());
    std::vector<VkDescriptorSetLayout> vkLayouts(mDescriptorSetLayouts.size());
    for (auto& [set, bindings] : setBindings)
    {
        mDescriptorSetLayouts[set] = std::make_unique<DescriptorSetLayout>(logicalDevice, bindings, 0);
        vkLayouts[set]             = mDescriptorSetLayouts[set]->getVkDescriptorLayout();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = static_cast<uint32>(vkLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts            = vkLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(mPushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges    = mPushConstantRanges.data();

    // create pipeline layout
    RAYCE_CHECK_VK(vkCreatePipelineLayout(mVkLogicalDeviceRef, &pipelineLayoutCreateInfo, nullptr, &mVkPipelineLayout), "Creating pipeline layout failed!");

    // create pipeline
    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo{};
    pipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount                   = static_cast<uint32>(vkShaderStageCreateInfos.size());
    pipelineCreateInfo.pStages                      = vkShaderStageCreateInfos.data();
    pipelineCreateInfo.groupCount                   = static_cast<uint32>(vkRTShaderGroupCreateInfos.size());
    pipelineCreateInfo.pGroups                      = vkRTShaderGroupCreateInfos.data();
    pipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
    pipelineCreateInfo.layout                       = mVkPipelineLayout;
    pipelineCreateInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex            = -1;

    RAYCE_CHECK_VK(vkCreateRayTracingPipelinesKHR(mVkLogicalDeviceRef, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mVkPipeline), "Creating raytracing pipeline failed!");

    // cleanup shader modulea
    for (auto& stageCreateInfo : vkShaderStageCreateInfos)
    {
        vkDestroyShaderModule(mVkLogicalDeviceRef, stageCreateInfo.module, nullptr);
    }

    RAYCE_LOG_INFO("Created raytracing pipeline!");
}

void Pipeline::mergePushConstantRanges()
{ // FIXME: Implement!!!!
}

Pipeline::~Pipeline()
{
    if (mVkPipelineLayout)
    {
        vkDestroyPipelineLayout(mVkLogicalDeviceRef, mVkPipelineLayout, nullptr);
    }
    if (mVkPipeline)
    {
        vkDestroyPipeline(mVkLogicalDeviceRef, mVkPipeline, nullptr);
    }
}
