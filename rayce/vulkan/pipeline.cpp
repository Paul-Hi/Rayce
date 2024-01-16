/// @file      pipeline.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "pipeline.hpp"

class GraphicsPipelineSettings;
class ComputePipelineSettings;
class RTPipelineSettings;

using namespace rayce;

std::shared_ptr<Pipeline> Pipeline::createGraphicsPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const GraphicsPipelineSettings& settings)
{
    return nullptr;
}

std::shared_ptr<Pipeline> Pipeline::createComputePipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const ComputePipelineSettings& settings)
{
    return nullptr;
}

std::shared_ptr<Pipeline> Pipeline::createRTPipeline(const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class Swapchain>& swapchain, const RTPipelineSettings& settings)
{
    return nullptr;
}
