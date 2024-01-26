/// @file      shaderModule.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <fstream>
#include <vulkan/device.hpp>
#include <vulkan/shaderModule.hpp>

using namespace rayce;

ShaderModule::ShaderModule(const std::unique_ptr<class Device>& logicalDevice, const str& spirvSourceFilename)
    : mVkLogicalDeviceRef(logicalDevice->getVkDevice())
{
    std::vector<char> spirvBinary = readFile(spirvSourceFilename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvBinary.size();
    createInfo.pCode    = reinterpret_cast<const uint32*>(spirvBinary.data());

    RAYCE_CHECK_VK(vkCreateShaderModule(mVkLogicalDeviceRef, &createInfo, nullptr, &mVkShaderModule), "Creating shader module failed!");

    RAYCE_LOG_INFO("Created shader module from %s!", spirvSourceFilename.c_str());
}

ShaderModule::~ShaderModule()
{
    if (mVkShaderModule)
    {
        vkDestroyShaderModule(mVkLogicalDeviceRef, mVkShaderModule, nullptr);
    }
}

VkPipelineShaderStageCreateInfo ShaderModule::createShaderStage(VkShaderStageFlagBits stage) const
{
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage  = stage;
    createInfo.module = mVkShaderModule;
    createInfo.pName  = "main";

    return createInfo;
}

std::vector<char> ShaderModule::readFile(const str& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    RAYCE_CHECK(file.is_open(), "Failed to open %s!", filename.c_str());

    std::streamsize fileSize = file.tellg();

    std::vector<char> spirvBinary(static_cast<ptr_size>(fileSize));

    file.seekg(0);
    file.read(spirvBinary.data(), fileSize);

    file.close();

    return spirvBinary;
}
