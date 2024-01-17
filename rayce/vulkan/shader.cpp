/// @file      shader.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <fstream>
#include <slang.h>
#include <vulkan/device.hpp>
#include <vulkan/shader.hpp>

using namespace rayce;

Shader::Shader(const str& slangFilename)
    : mSlangFilename(slangFilename)
    , mReflected(false)
{
}

static SlangStage getSlangStage(const ShaderStage& stage)
{
    switch (stage)
    {
    case ShaderStage::VertexStage:
        return SLANG_STAGE_VERTEX;
    case ShaderStage::FragmentStage:
        return SLANG_STAGE_FRAGMENT;
    case ShaderStage::GeometryStage:
        return SLANG_STAGE_GEOMETRY;
    case ShaderStage::ComputeStage:
        return SLANG_STAGE_COMPUTE;
    case ShaderStage::RayGenerationStage:
        return SLANG_STAGE_RAY_GENERATION;
    case ShaderStage::ClosestHitStage:
        return SLANG_STAGE_CLOSEST_HIT;
    case ShaderStage::MissStage:
        return SLANG_STAGE_MISS;
    case ShaderStage::AnyHitStage:
        return SLANG_STAGE_ANY_HIT;
    case ShaderStage::IntersectionStage:
        return SLANG_STAGE_INTERSECTION;
    case ShaderStage::CallableStage:
        return SLANG_STAGE_CALLABLE;
    default:
        RAYCE_ASSERT(false, "Unknown ShaderStage!");
    }
}

bool Shader::compileAndReflect(const ShaderSpecialization& shaderSpecialization)
{
    mShaderSpecialization = shaderSpecialization;

    SlangSession* session = spCreateSession();

    constexpr SlangCompileTarget compileTarget = SLANG_SPIRV; // Always SpirV
    constexpr SlangSourceLanguage source       = SLANG_SOURCE_LANGUAGE_SLANG;

    SlangCompileRequest* request = spCreateCompileRequest(session);
    int target                   = spAddCodeGenTarget(request, compileTarget);
    SlangProfileID profileID     = spFindProfile(session, "glsl_460");
    spSetTargetProfile(request, target, profileID);

    spAddSearchPath(request, SLANG_SHADER_BASEPATH);
    spSetDebugInfoLevel(request, SLANG_DEBUG_INFO_LEVEL_STANDARD);
    spSetDiagnosticFlags(request, SLANG_DIAGNOSTIC_FLAG_TREAT_WARNINGS_AS_ERRORS);
    spSetOptimizationLevel(request, SLANG_OPTIMIZATION_LEVEL_DEFAULT); // FIXME: Revisit Higher Levels
    spSetMatrixLayoutMode(request, SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
    spSetTargetForceGLSLScalarBufferLayout(request, target, true);

    // add the compile macros
    for (auto& macro : shaderSpecialization.macros)
    {
        spAddPreprocessorDefine(request, macro.name.c_str(), macro.hasValue ? macro.value.c_str() : "");
    }

    int32 translationUnit = spAddTranslationUnit(request, source, mSlangFilename.c_str());
    spAddTranslationUnitSourceFile(request, translationUnit, mSlangFilename.c_str());

    int entryPointIndex = spAddEntryPoint(request, translationUnit, mShaderSpecialization.entryPoint.c_str(), getSlangStage(mShaderSpecialization.stage));

    // Compile
    SlangResult result           = spCompile(request);
    const char* diagnosticOutput = spGetDiagnosticOutput(request);

    if (result != SLANG_OK)
    {
        std::istringstream ss(diagnosticOutput);
        std::string line;
        while (std::getline(ss, line))
        {
            RAYCE_LOG_ERROR("Slang compilation of %s failed: %s", mSlangFilename.c_str(), line.c_str());
        }
        return false;
    }

    ptr_size dataSize = 0;
    const void* data  = spGetEntryPointCode(request, entryPointIndex, &dataSize);

    // Reallocate - Slang deletes its stuff
    RAYCE_ASSERT(dataSize > 0 && dataSize % 4 == 0); // SpirV
    mSpirvBinary.resize(dataSize);
    std::memcpy(mSpirvBinary.data(), data, dataSize);

    // FIXME Next: Still to fill! We should also see to use more c++ API to not create a global session thing everytime ...
    mStage;
    mBindingMask;
    mDescriptorTypes;
    mPushConstantRanges;
    mVertexInputs;
    mLocalSizeX;
    mLocalSizeY;
    mLocalSizeZ;
    mReflected;

    spDestroyCompileRequest(request);

    spDestroySession(session);

    return true;
}

VkShaderModule Shader::createShaderModule(const std::unique_ptr<class Device>& logicalDevice) const
{
    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32>(mSpirvBinary.size());
    createInfo.pCode    = reinterpret_cast<const uint32*>(mSpirvBinary.data());

    RAYCE_CHECK_VK(vkCreateShaderModule(logicalDevice->getVkDevice(), &createInfo, nullptr, &shaderModule), "Creating shader module failed!");

    RAYCE_LOG_INFO("Created shader module from %s!", mSlangFilename.c_str());

    return shaderModule;
}
