/// @file      shaderReflection.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <slang.h>
#include <vulkan/shaderReflection.hpp>

using namespace rayce;

ShaderReflection::ShaderReflection(slang::ShaderReflection* slangTopLevelReflection, slang::EntryPointLayout* slangEntryPointLayout)
    : mTopLevelReflection(slangTopLevelReflection)
    , mEntryPointLayout(slangEntryPointLayout)
{
    // global parameters
    slang::TypeLayoutReflection* globalParameterLayout = mTopLevelReflection->getGlobalParamsTypeLayout();
    // could be either a struct type or a struct type wrapped in a Buffer
    // in the second case we want to unwrap the struct type
    if (slang::TypeLayoutReflection* elementTypeLayout = globalParameterLayout->getElementTypeLayout())
    {
        globalParameterLayout = elementTypeLayout;
    }

    ptr_size gobalParameterSize = globalParameterLayout->getSize(SlangParameterCategory(slang::ParameterCategory::Uniform));

    // falcor ProgramReflection.cpp line 1508
    // ref<ReflectionStructType> pGlobalStruct     = ReflectionStructType::create(slangGlobalParamsSize, "", nullptr);
    // ref<ParameterBlockReflection> pDefaultBlock = ParameterBlockReflection::createEmpty(pProgramVersion);
    // pDefaultBlock->setElementType(pGlobalStruct);
    //
    // ReflectionStructType::BuildState buildState;
    // for (uint32_t i = 0; i < pSlangReflector->getParameterCount(); i++)
    // {
    //     VariableLayoutReflection* pSlangLayout = pSlangReflector->getParameterByIndex(i);
    //
    //     ref<ReflectionVar> pVar =
    //         reflectTopLevelVariable(pSlangLayout, pGlobalStruct->getResourceRangeCount(), pDefaultBlock.get(), pProgramVersion);
    //     if (pVar)
    //         pGlobalStruct->addMember(pVar, buildState);
    // }
    //
    // pDefaultBlock->finalize();
    // setDefaultParameterBlock(pDefaultBlock);
}
