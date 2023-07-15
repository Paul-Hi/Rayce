/// @file      shaderModule.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SHADER_MODULE_HPP
#define SHADER_MODULE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT ShaderModule
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(ShaderModule)

        ShaderModule(const std::unique_ptr<class Device>& logicalDevice, const str& spirvSourceFilename);
        ~ShaderModule();

        VkShaderModule getVkShaderModule() const
        {
            return mVkShaderModule;
        }

        VkPipelineShaderStageCreateInfo createShaderStage(VkShaderStageFlagBits stage) const;

    private:
        VkShaderModule mVkShaderModule;
        VkDevice mVkLogicalDeviceRef;

        std::vector<char> readFile(const str& filename);
    };
} // namespace rayce

#endif // SHADER_MODULE_HPP
