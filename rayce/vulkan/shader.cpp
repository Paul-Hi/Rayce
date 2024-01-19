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
    case ShaderStage::StageCount:
    default:
        RAYCE_ASSERT(false, "Unknown ShaderStage!");
        return SLANG_STAGE_NONE;
    }
}

bool Shader::compileAndReflect(const std::unique_ptr<Device>& logicalDevice, const ShaderSpecialization& shaderSpecialization)
{
    constexpr SlangCompileTarget compileTarget = SLANG_SPIRV; // Always SpirV
    constexpr SlangSourceLanguage source       = SLANG_SOURCE_LANGUAGE_SLANG;

    mShaderSpecialization = shaderSpecialization;

    SlangSession* globalSession = logicalDevice->getSlangGlobalSession();
    RAYCE_ASSERT(globalSession);

    slang::SessionDesc sessionDesc;
    sessionDesc.searchPathCount          = 1;
    std::vector<const char*> searchPaths = { SLANG_SHADER_BASEPATH };
    sessionDesc.searchPaths              = searchPaths.data();

    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    slang::TargetDesc targetDesc;

    targetDesc.format                      = compileTarget;
    targetDesc.forceGLSLScalarBufferLayout = true;
    targetDesc.profile                     = globalSession->findProfile("glsl_460");
    targetDesc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY; // FIXME Next: For now, since we get a strange glslang error for raygen

    std::vector<slang::PreprocessorMacroDesc> slangDefines;

    for (auto& macro : shaderSpecialization.macros)
    {
        slang::PreprocessorMacroDesc define;
        define.name  = macro.name.c_str();
        define.value = macro.hasValue ? macro.value.c_str() : nullptr;
        slangDefines.push_back(define);
    }

    sessionDesc.preprocessorMacros     = slangDefines.data();
    sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(slangDefines.size());

    sessionDesc.targets     = &targetDesc;
    sessionDesc.targetCount = 1;

    slang::ISession* session;
    globalSession->createSession(sessionDesc, &session);
    RAYCE_ASSERT(session);

    SlangCompileRequest* request;
    session->createCompileRequest(&request);
    RAYCE_ASSERT(request);

    request->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_STANDARD);
    request->setDiagnosticFlags(SLANG_DIAGNOSTIC_FLAG_TREAT_WARNINGS_AS_ERRORS);
    request->setOptimizationLevel(SLANG_OPTIMIZATION_LEVEL_DEFAULT); // FIXME: Revisit Higher Levels

    int32 translationUnit = request->addTranslationUnit(source, mSlangFilename.c_str());
    request->addTranslationUnitSourceFile(translationUnit, mSlangFilename.c_str());

    int entryPointIndex = request->addEntryPoint(translationUnit, mShaderSpecialization.entryPoint.c_str(), getSlangStage(mShaderSpecialization.stage));

    // Compile
    SlangResult result           = request->compile();
    const char* diagnosticOutput = request->getDiagnosticOutput();

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
    const void* data  = request->getEntryPointCode(entryPointIndex, &dataSize);

    // Reallocate - Slang deletes its stuff
    RAYCE_ASSERT(dataSize > 0 && dataSize % 4 == 0); // SpirV
    mSpirvBinary.resize(dataSize);
    std::memcpy(mSpirvBinary.data(), data, dataSize);

    // FIXME Next: Still to fill! We should also see to use more c++ API
    mStage;

    slang::ShaderReflection* shaderReflection = slang::ShaderReflection::get(request);

    slang::EntryPointReflection* entryPointReflection = shaderReflection->getEntryPointByIndex(entryPointIndex);

    if (shaderSpecialization.stage == ShaderStage::ComputeStage)
    {
        SlangUInt sizes[3];
        entryPointReflection->getComputeThreadGroupSize(3, sizes);
        mLocalSizeX = sizes[0];
        mLocalSizeY = sizes[1];
        mLocalSizeZ = sizes[2];
    }

    uint32 entryPointParams = entryPointReflection->getParameterCount();

    for (uint32 i = 0; i < entryPointParams; ++i)
    {
        slang::VariableLayoutReflection* parameterReflection = entryPointReflection->getParameterByIndex(i);

        uint32 binding = parameterReflection->getBindingIndex(); // Binding
        uint32 set     = parameterReflection->getBindingSpace(); // Set

        slang::TypeLayoutReflection* typeLayout = parameterReflection->getTypeLayout();

        slang::ParameterCategory parameterCategory = typeLayout->getParameterCategory();

        slang::TypeReflection* typeReflection = typeLayout->getType();

        slang::TypeReflection::Kind kind = typeReflection->getKind();

        RAYCE_LOG_INFO("EntryPoint: At (set/binding) = (%d/%d) we got a %s!", set, binding, typeReflection->getName());
    }

    uint32 shaderParams = shaderReflection->getParameterCount();

    for (uint32 i = 0; i < shaderParams; ++i)
    {
        slang::VariableLayoutReflection* parameterReflection = shaderReflection->getParameterByIndex(i);

        uint32 binding = parameterReflection->getBindingIndex(); // Binding
        uint32 set     = parameterReflection->getBindingSpace(); // Set

        slang::TypeLayoutReflection* typeLayout = parameterReflection->getTypeLayout();

        slang::ParameterCategory parameterCategory = typeLayout->getParameterCategory();

        slang::TypeReflection* typeReflection = typeLayout->getType();

        slang::TypeReflection::Kind kind = typeReflection->getKind();

        RAYCE_LOG_INFO("Shader: At (set/binding) = (%d/%d) we got a %s!", set, binding, typeReflection->getName());
    }

    mBindingMask;
    mDescriptorTypes;
    mPushConstantRanges;
    mVertexInputs;
    mReflected;

    spDestroyCompileRequest(request);

    session->release();

    return true;
}

VkShaderModule Shader::createShaderModule(const std::unique_ptr<Device>& logicalDevice) const
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
