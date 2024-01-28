/// @file      shader.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef SHADER_HPP
#define SHADER_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT ShaderMacro
    {
        str name;
        str value;
        bool hasValue;
    };

    enum struct RAYCE_API_EXPORT ShaderStage : uint32
    {
        VertexStage,
        FragmentStage,
        GeometryStage,
        ComputeStage,
        RayGenerationStage,
        ClosestHitStage,
        MissStage,
        AnyHitStage,
        IntersectionStage,
        CallableStage,
        StageCount
    };

    struct RAYCE_API_EXPORT ShaderSpecialization
    {
        str entryPoint;
        ShaderStage stage;
        std::vector<ShaderMacro> macros = {};
    };

    class RAYCE_API_EXPORT Shader
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Shader)

        Shader(const str& slangFilename);
        ~Shader() = default;

        bool compileAndReflect(const std::unique_ptr<class Device>& logicalDevice, const ShaderSpecialization& shaderSpecialization);
        VkShaderModule createShaderModule(const std::unique_ptr<class Device>& logicalDevice) const;

        bool compiled() { return mReflected; }

    private:
        friend class Pipeline;

        str mSlangFilename;

        bool mReflected = false;

        std::vector<uint32> mSpirvBinary = {};

        VkShaderStageFlagBits mStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        ShaderSpecialization mShaderSpecialization;

        std::vector<std::shared_ptr<class ShaderParameterBlockReflection>> mReflection;
    };
} // namespace rayce

#endif // SHADER_HPP
