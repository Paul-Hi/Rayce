/// @file      shader.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
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

static VkShaderStageFlagBits getVkStage(const ShaderStage& stage)
{
    switch (stage)
    {
    case ShaderStage::VertexStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FragmentStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::GeometryStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::ComputeStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::RayGenerationStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case ShaderStage::ClosestHitStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case ShaderStage::MissStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
    case ShaderStage::AnyHitStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case ShaderStage::IntersectionStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    case ShaderStage::CallableStage:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    case ShaderStage::StageCount:
    default:
        RAYCE_ASSERT(false, "Unknown ShaderStage!");
        return VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

static str getParameterCategoryName(slang::ParameterCategory category)
{
    switch (category)
    {

    // TODO: these aren't scoped...
    case slang::ParameterCategory::None:
        return "None";
    case slang::ParameterCategory::Mixed:
        return "Mixed";
    case slang::ParameterCategory::ConstantBuffer:
        return "ConstantBuffer";
    case slang::ParameterCategory::ShaderResource:
        return "ShaderResource";
    case slang::ParameterCategory::UnorderedAccess:
        return "UnorderedAccess";
    case slang::ParameterCategory::VaryingInput:
        return "VaryingInput";
    case slang::ParameterCategory::VaryingOutput:
        return "VaryingOutput";
    case slang::ParameterCategory::SamplerState:
        return "SamplerState";
    case slang::ParameterCategory::Uniform:
        return "Uniform";
    case slang::ParameterCategory::DescriptorTableSlot:
        return "DescriptorTableSlot";
    case slang::ParameterCategory::SpecializationConstant:
        return "SpecializationConstant";
    case slang::ParameterCategory::PushConstantBuffer:
        return "PushConstantBuffer";
    case slang::ParameterCategory::RegisterSpace:
        return "RegisterSpace";
    case slang::ParameterCategory::GenericResource:
        return "GenericResource";
    case slang::ParameterCategory::RayPayload:
        return "RayPayload";
    case slang::ParameterCategory::HitAttributes:
        return "HitAttributes";
    case slang::ParameterCategory::CallablePayload:
        return "CallablePayload";
    case slang::ParameterCategory::ShaderRecord:
        return "ShaderRecord";
    case slang::ParameterCategory::ExistentialTypeParam:
        return "ExistentialTypeParam";
    case slang::ParameterCategory::ExistentialObjectParam:
        return "ExistentialObjectParam";
    case slang::ParameterCategory::SubElementRegisterSpace:
        return "SubElementRegisterSpace";
    default:
        return "";
    }
}

static VkFormat getVertexAttribFormat(slang::TypeReflection::ScalarType scalarType, uint32 elements)
{
    switch (scalarType)
    {
    case slang::TypeReflection::ScalarType::Int8:
        return elements == 1 ? VK_FORMAT_R8_SINT : (elements == 2 ? VK_FORMAT_R8G8_SINT : (elements == 3 ? VK_FORMAT_R8G8B8_SINT : VK_FORMAT_R8G8B8A8_SINT));
    case slang::TypeReflection::ScalarType::Int16:
        return elements == 1 ? VK_FORMAT_R16_SINT : (elements == 2 ? VK_FORMAT_R16G16_SINT : (elements == 3 ? VK_FORMAT_R16G16B16_SINT : VK_FORMAT_R16G16B16A16_SINT));
    case slang::TypeReflection::ScalarType::Int32:
        return elements == 1 ? VK_FORMAT_R32_SINT : (elements == 2 ? VK_FORMAT_R32G32_SINT : (elements == 3 ? VK_FORMAT_R32G32B32_SINT : VK_FORMAT_R32G32B32A32_SINT));
    case slang::TypeReflection::ScalarType::Int64:
        return elements == 1 ? VK_FORMAT_R64_SINT : (elements == 2 ? VK_FORMAT_R64G64_SINT : (elements == 3 ? VK_FORMAT_R64G64B64_SINT : VK_FORMAT_R64G64B64A64_SINT));
        break;
    case slang::TypeReflection::ScalarType::UInt8:
        return elements == 1 ? VK_FORMAT_R8_UINT : (elements == 2 ? VK_FORMAT_R8G8_UINT : (elements == 3 ? VK_FORMAT_R8G8B8_UINT : VK_FORMAT_R8G8B8A8_UINT));
    case slang::TypeReflection::ScalarType::UInt16:
        return elements == 1 ? VK_FORMAT_R16_UINT : (elements == 2 ? VK_FORMAT_R16G16_UINT : (elements == 3 ? VK_FORMAT_R16G16B16_UINT : VK_FORMAT_R16G16B16A16_UINT));
    case slang::TypeReflection::ScalarType::UInt32:
        return elements == 1 ? VK_FORMAT_R32_UINT : (elements == 2 ? VK_FORMAT_R32G32_UINT : (elements == 3 ? VK_FORMAT_R32G32B32_UINT : VK_FORMAT_R32G32B32A32_UINT));
    case slang::TypeReflection::ScalarType::UInt64:
        return elements == 1 ? VK_FORMAT_R64_UINT : (elements == 2 ? VK_FORMAT_R64G64_UINT : (elements == 3 ? VK_FORMAT_R64G64B64_UINT : VK_FORMAT_R64G64B64A64_UINT));
        break;
    case slang::TypeReflection::ScalarType::Float16:
        return elements == 1 ? VK_FORMAT_R16_SFLOAT : (elements == 2 ? VK_FORMAT_R16G16_SFLOAT : (elements == 3 ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT));
    case slang::TypeReflection::ScalarType::Float32:
        return elements == 1 ? VK_FORMAT_R32_SFLOAT : (elements == 2 ? VK_FORMAT_R32G32_SFLOAT : (elements == 3 ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT));
    case slang::TypeReflection::ScalarType::Float64:
        return elements == 1 ? VK_FORMAT_R64_SFLOAT : (elements == 2 ? VK_FORMAT_R64G64_SFLOAT : (elements == 3 ? VK_FORMAT_R64G64B64_SFLOAT : VK_FORMAT_R64G64B64A64_SFLOAT));
    case slang::TypeReflection::ScalarType::None:
    case slang::TypeReflection::ScalarType::Void:
    case slang::TypeReflection::ScalarType::Bool:
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static uint32 getScalarByteSize(slang::TypeReflection::ScalarType scalarType)
{
    switch (scalarType)
    {
    case slang::TypeReflection::ScalarType::Int8:
    case slang::TypeReflection::ScalarType::UInt8:
        return 1;
    case slang::TypeReflection::ScalarType::Int16:
    case slang::TypeReflection::ScalarType::UInt16:
    case slang::TypeReflection::ScalarType::Float16:
        return 2;
    case slang::TypeReflection::ScalarType::Int32:
    case slang::TypeReflection::ScalarType::UInt32:
    case slang::TypeReflection::ScalarType::Float32:
        return 4;
    case slang::TypeReflection::ScalarType::Int64:
    case slang::TypeReflection::ScalarType::UInt64:
    case slang::TypeReflection::ScalarType::Float64:
        return 8;
    case slang::TypeReflection::ScalarType::None:
    case slang::TypeReflection::ScalarType::Void:
    case slang::TypeReflection::ScalarType::Bool:
    default:
        return 0;
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

    mStage = getVkStage(mShaderSpecialization.stage);

    slang::ShaderReflection* shaderReflection = slang::ShaderReflection::get(request);

    slang::EntryPointReflection* entryPointReflection = shaderReflection->getEntryPointByIndex(entryPointIndex);

    if (shaderSpecialization.stage == ShaderStage::ComputeStage)
    {
        SlangUInt sizes[3];
        entryPointReflection->getComputeThreadGroupSize(3, sizes);
        mLocalSizeX = static_cast<uint32>(sizes[0]);
        mLocalSizeY = static_cast<uint32>(sizes[1]);
        mLocalSizeZ = static_cast<uint32>(sizes[2]);
    }

    uint32 entryPointParams = entryPointReflection->getParameterCount();

    for (uint32 i = 0; i < entryPointParams; ++i) // In theory we have to check for uniforms I guess - but lets only handle the vertex input?
    {
        slang::VariableLayoutReflection* parameterReflection = entryPointReflection->getParameterByIndex(i);

        uint32 binding = parameterReflection->getBindingIndex(); // Binding
        uint32 set     = parameterReflection->getBindingSpace(); // Set

        slang::TypeLayoutReflection* typeLayout = parameterReflection->getTypeLayout();

        slang::ParameterCategory parameterCategory = typeLayout->getParameterCategory();

        slang::TypeReflection* typeReflection = typeLayout->getType();

        //  Vertex Inputs
        mVertexInputBinding           = {};
        mVertexInputBinding.binding   = 0;
        mVertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // FIXME: Correct?

        uint32 attributeBindings = static_cast<uint32>(typeLayout->getBindingRangeCount());
        mVertexInputAttributes.resize(attributeBindings);

        mVertexInputBinding.stride = 0;
        for (uint32 a = 0; a < attributeBindings; ++a)
        {
            slang::VariableLayoutReflection* bindingReflection                = typeLayout->getFieldByIndex(a);
            VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
            vertexInputAttributeDescription.binding                           = 0;
            vertexInputAttributeDescription.location                          = a;
            vertexInputAttributeDescription.offset                            = static_cast<uint32>(bindingReflection->getOffset());

            slang::TypeLayoutReflection* bindingTypeLayout = bindingReflection->getTypeLayout();

            slang::TypeReflection* bindingTypeReflection = bindingTypeLayout->getType();

            slang::TypeReflection::ScalarType scalarType = bindingTypeReflection->getElementType()->getScalarType();
            uint32 elements                              = static_cast<uint32>(bindingTypeReflection->getElementCount());

            vertexInputAttributeDescription.format = getVertexAttribFormat(scalarType, elements);

            mVertexInputAttributes[a] = vertexInputAttributeDescription;

            mVertexInputBinding.stride += elements * getScalarByteSize(scalarType);
        }

        RAYCE_LOG_DINFO("EntryPoint: At (set/binding) = (%d/%d) we got a %s %s named %s!", set, binding, getParameterCategoryName(parameterCategory).c_str(), typeReflection->getName(), parameterReflection->getName());
    }

    uint32 shaderParams = shaderReflection->getParameterCount();

    for (uint32 i = 0; i < shaderParams; ++i)
    {
        slang::VariableLayoutReflection* parameterReflection = shaderReflection->getParameterByIndex(i);

        uint32 binding = parameterReflection->getBindingIndex(); // Binding
        uint32 set     = parameterReflection->getBindingSpace(); // Set

        slang::TypeLayoutReflection* typeLayout = parameterReflection->getTypeLayout();

        slang::ParameterCategory parameterCategory = typeLayout->getParameterCategory();

        if (parameterCategory == slang::ParameterCategory::PushConstantBuffer)
        {
            // push constants - will get merged later
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = mStage;
            pushConstantRange.offset     = static_cast<uint32>(parameterReflection->getOffset()); // FIXME: Correct?
            pushConstantRange.size       = 0;                                                     // FIXME: FUCK!

            mPushConstantRanges.push_back(pushConstantRange);
            std::cout << pushConstantRange.offset << " offset\n";
            std::cout << pushConstantRange.size << " size\n";
        }
        else if (parameterCategory == slang::ParameterCategory::DescriptorTableSlot)
        {
        }
        else
        {
            RAYCE_LOG_ERROR("We do not support %s!", getParameterCategoryName(parameterCategory).c_str());
        }

        slang::TypeReflection* typeReflection = typeLayout->getType();

        RAYCE_LOG_INFO("Shader: At (set/binding) = (%d/%d) we got a %s %s named %s!", set, binding, getParameterCategoryName(parameterCategory).c_str(), typeReflection->getName(), parameterReflection->getName());
    }

    // FIXME Next: Still to fill!
    mDescriptorSetLayoutBindings;

    mReflected = true;

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
