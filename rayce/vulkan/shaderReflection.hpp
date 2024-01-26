/// @file      shaderReflection.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef SHADER_REFLECTION_HPP
#define SHADER_REFLECTION_HPP

namespace slang
{
    struct ShaderReflection;
    struct EntryPointReflection;
    typedef EntryPointReflection EntryPointLayout;
}

// This is a mixture between the stuff Falcor does and a lot of slang stuff...
namespace rayce
{
    enum class ShaderResourceDescriptorType
    {
        Sampler, // Sampler
        Texture, // Sampled Image
        ConstantBuffer, // Uniform Buffer
        ParameterBlock, //
        TypedBuffer,
        RawBuffer,
        CombinedTextureSampler,
        InputRenderTarget,
        InlineUniformData,
        RayTracingAccelerationStructure,
        VaryingInput,
        VaryingOutput,
        ExistentialValue,
        PushConstant,

        MutableFlag,

        MutableTexture,
        MutableTypedBuffer,
        MutableRawBuffer,

        BaseMask,
        ExtMask,

    };

    class ShaderReflectionArrayType;
    class ShaderReflectionStructType;
    class ShaderReflectionBasicType;
    class ShaderReflectionResourceType;
    class ShaderReflectionInterfaceType;

    class ShaderReflectionVariable;
    class ShaderReflectionMemberOffsetAndType;

    class RAYCE_API_EXPORT ShaderReflectionType
    {
    public:
        virtual ~ShaderReflectionType() = default;

        enum class Kind : byte
        {
            Array,
            Struct,
            Basic,
            Resource,
            Interface
        };

        Kind getKind() const { return mKind; }

        const ShaderReflectionArrayType* thisAsArrayType() const;
        const ShaderReflectionStructType* thisAsStructType() const;
        const ShaderReflectionBasicType* thisAsBasicType() const;
        const ShaderReflectionResourceType* thisAsResourceType() const;
        const ShaderReflectionInterfaceType* thisAsInterfaceType() const;

        const ShaderReflectionType* getArrayElementType() const;
        uint32 getArrayElementCount() const;

        ptr_size getSizeInBytes() const { return mSize; }

        std::shared_ptr<ShaderReflectionVariable> getMember(const std::string& name) const;
        ShaderReflectionMemberOffsetAndType getMemberOffsetAndType(const std::string& name) const;
        ShaderReflectionMemberOffsetAndType getMemberOffsetAndTypeAt(ptr_size byteOffset) const;

        virtual bool operator==(const ShaderReflectionType& other) const = 0;
        bool operator!=(const ShaderReflectionType& other) { return !(*this == other); }

        struct DescriptorRange
        {
            ShaderResourceDescriptorType descriptorType;

            uint32 descriptorCount;
        };

    private:
        Kind mKind;
        ptr_size mSize;
    };

    class RAYCE_API_EXPORT ShaderReflection
    {
    public:
        ShaderReflection(slang::ShaderReflection* slangTopLevelReflection, slang::EntryPointLayout* slangEntryPointLayout);
        ~ShaderReflection() = default;

    private:
        slang::ShaderReflection* mTopLevelReflection;
        slang::EntryPointLayout* mEntryPointLayout;

        VkShaderStageFlagBits mStage                                                              = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        std::vector<std::pair<uint32, VkDescriptorSetLayoutBinding>> mDescriptorSetLayoutBindings = {};
        std::vector<VkPushConstantRange> mPushConstantRanges                                      = {}; // push constants available at this stage
        VkVertexInputBindingDescription mVertexInputBinding                                       = {};
        std::vector<VkVertexInputAttributeDescription> mVertexInputAttributes                     = {};
        // work packages for compute
        uint32 mLocalSizeX = 0, mLocalSizeY = 0, mLocalSizeZ = 0;
    };
} // namespace rayce

#endif // SHADER_REFLECTION_HPP
