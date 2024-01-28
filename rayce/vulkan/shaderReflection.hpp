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
    struct TypeLayoutReflection;
}

// This is a mixture between the stuff Falcor does and a lot of slang stuff...
namespace rayce
{
    enum class EShaderResourceDescriptorType
    {
        Sampler,
        Texture,
        MutableTexture,
        RawBuffer,
        MutableRawBuffer,
        TypedBuffer,
        MutableTypedBuffer,
        StructuredBuffer,
        MutableStructuredBuffer,
        ConstantBuffer,
        CombinedTextureSampler,
        PushConstant,
        RaytracingAccelerationStructure,

        Count
    };

    class Shader;

    class ShaderReflectionArrayType;
    class ShaderReflectionStructType;
    class ShaderReflectionBasicType;
    class ShaderReflectionResourceType;
    class ShaderReflectionInterfaceType;

    class ShaderReflectionVariable;

    class ShaderParameterBlockReflection;

    class RAYCE_API_EXPORT ShaderReflectionUniformVariableOffset
    {
    public:
        explicit ShaderReflectionUniformVariableOffset(ptr_size offset)
            : mByteOffset(static_cast<uint32>(offset))
        {
        }

        enum EZero
        {
            Zero
        };

        enum EInvalid
        {
            Invalid
        };

        ShaderReflectionUniformVariableOffset(EZero)
            : mByteOffset(0)
        {
        }

        ShaderReflectionUniformVariableOffset(EInvalid = EInvalid::Invalid)
            : mByteOffset(static_cast<uint32>(-1))
        {
        }

        uint32 getByteOffset() const
        {
            return mByteOffset;
        }

        bool valid() const { return mByteOffset != static_cast<uint32>(-1); }

        bool operator==(const ShaderReflectionUniformVariableOffset& other) const { return mByteOffset == other.mByteOffset; }
        bool operator!=(const ShaderReflectionUniformVariableOffset& other) const { return !(*this == other); }
        bool operator==(EInvalid) const { return !valid(); }
        bool operator!=(EInvalid) const { return valid(); }

        ShaderReflectionUniformVariableOffset operator+(size_t offset) const
        {
            if (!valid())
            {
                return EInvalid::Invalid;
            }

            return ShaderReflectionUniformVariableOffset(mByteOffset + offset);
        }

        ShaderReflectionUniformVariableOffset operator+(ShaderReflectionUniformVariableOffset other) const
        {
            if (!valid())
            {
                return EInvalid::Invalid;
            }
            if (!other.valid())
            {
                return EInvalid::Invalid;
            }

            return ShaderReflectionUniformVariableOffset(mByteOffset + other.mByteOffset);
        }

    private:
        uint32 mByteOffset = static_cast<uint32>(-1);
    };

    class RAYCE_API_EXPORT ShaderReflectionResourceVariableOffset
    {
    public:
        explicit ShaderReflectionResourceVariableOffset(uint32 rangeIndex, uint32 arrayIndex)
            : mRangeIndex(rangeIndex)
            , mArrayIndex(arrayIndex)
        {
        }

        explicit ShaderReflectionResourceVariableOffset(uint32 rangeIndex)
            : mRangeIndex(rangeIndex)
            , mArrayIndex(0)
        {
        }

        enum EZero
        {
            Zero
        };

        enum EInvalid
        {
            Invalid
        };

        ShaderReflectionResourceVariableOffset(EZero)
            : mRangeIndex(static_cast<uint32>(0))
            , mArrayIndex(static_cast<uint32>(0))
        {
        }

        ShaderReflectionResourceVariableOffset(EInvalid = EInvalid::Invalid)
            : mRangeIndex(static_cast<uint32>(-1))
            , mArrayIndex(static_cast<uint32>(-1))
        {
        }

        uint32 getRangeIndex() const
        {
            return mRangeIndex;
        }

        uint32 getArrayIndex() const
        {
            return mArrayIndex;
        }

        bool valid() const { return mRangeIndex != static_cast<uint32>(-1); }

        bool operator==(const ShaderReflectionResourceVariableOffset& other) const { return mRangeIndex == other.mRangeIndex && mArrayIndex == other.mArrayIndex; }
        bool operator!=(const ShaderReflectionResourceVariableOffset& other) const { return !(*this == other); }
        bool operator==(EInvalid) const { return !valid(); }
        bool operator!=(EInvalid) const { return valid(); }

        ShaderReflectionResourceVariableOffset operator+(ShaderReflectionResourceVariableOffset other) const
        {
            if (!valid())
            {
                return EInvalid::Invalid;
            }
            if (!other.valid())
            {
                return EInvalid::Invalid;
            }

            return ShaderReflectionResourceVariableOffset(mRangeIndex + other.mRangeIndex, mArrayIndex + other.mArrayIndex);
        }

    private:
        uint32 mRangeIndex = static_cast<uint32>(-1);
        uint32 mArrayIndex = static_cast<uint32>(-1);
    };

    class RAYCE_API_EXPORT ShaderReflectionVariableOffset
    {
    public:
        explicit ShaderReflectionVariableOffset(ShaderReflectionUniformVariableOffset uniformOffset, ShaderReflectionResourceVariableOffset resourceOffset)
            : mUniformOffset(uniformOffset)
            , mResourceOffset(resourceOffset)
        {
        }

        enum EZero
        {
            Zero
        };

        enum EInvalid
        {
            Invalid
        };

        ShaderReflectionVariableOffset(EZero)
            : mUniformOffset(ShaderReflectionUniformVariableOffset::Zero)
            , mResourceOffset(ShaderReflectionResourceVariableOffset::Zero)
        {
        }

        ShaderReflectionVariableOffset(EInvalid = EInvalid::Invalid)
            : mUniformOffset(ShaderReflectionUniformVariableOffset::Invalid)
            , mResourceOffset(ShaderReflectionResourceVariableOffset::Invalid)
        {
        }

        ShaderReflectionUniformVariableOffset getUniform() const
        {
            return mUniformOffset;
        }

        ShaderReflectionResourceVariableOffset getResource() const
        {
            return mResourceOffset;
        }

        operator ShaderReflectionResourceVariableOffset() const { return mResourceOffset; }

        bool valid() const { return mUniformOffset.valid(); }

        bool operator==(const ShaderReflectionVariableOffset& other) const { return mUniformOffset == other.mUniformOffset && mResourceOffset == other.mResourceOffset; }
        bool operator!=(const ShaderReflectionVariableOffset& other) const { return !(*this == other); }
        bool operator==(EInvalid) const { return !valid(); }
        bool operator!=(EInvalid) const { return valid(); }

        ShaderReflectionVariableOffset operator+(ShaderReflectionVariableOffset other) const
        {
            if (!valid())
            {
                return EInvalid::Invalid;
            }
            if (!other.valid())
            {
                return EInvalid::Invalid;
            }

            return ShaderReflectionVariableOffset(mUniformOffset + other.mUniformOffset, mResourceOffset + other.mResourceOffset);
        }

        uint32 getByteOffset() const
        {
            return mUniformOffset.getByteOffset();
        }

        uint32 getRangeIndex() const
        {
            return mResourceOffset.getRangeIndex();
        }

        uint32 getArrayIndex() const
        {
            return mResourceOffset.getArrayIndex();
        }

    private:
        ShaderReflectionUniformVariableOffset mUniformOffset;
        ShaderReflectionResourceVariableOffset mResourceOffset;
    };

    class RAYCE_API_EXPORT ShaderReflectionVariableOffsetAndType : ShaderReflectionVariableOffset
    {
    public:
        ShaderReflectionVariableOffsetAndType(EInvalid = EInvalid::Invalid) {}

        const std::shared_ptr<ShaderReflectionType>& getReflectionType() const { return mReflectionType; }

        bool valid() const { return mReflectionType != nullptr; }

        ShaderReflectionVariableOffsetAndType operator[](const str& name) const;

        ShaderReflectionVariableOffsetAndType operator[](ptr_size index) const;

        ShaderReflectionVariableOffsetAndType(const std::shared_ptr<ShaderReflectionType>& reflectionType, ShaderReflectionVariableOffset offset);

    private:
        std::shared_ptr<ShaderReflectionType> mReflectionType;
    };

    class RAYCE_API_EXPORT ShaderReflectionType
    {
    public:
        virtual ~ShaderReflectionType() = default;

        enum class EKind : byte
        {
            Array,
            Struct,
            Basic,
            Resource,
            Interface
        };

        EKind getKind() const { return mKind; }

        const ShaderReflectionArrayType* thisAsArrayType() const;
        const ShaderReflectionStructType* thisAsStructType() const;
        const ShaderReflectionBasicType* thisAsBasicType() const;
        const ShaderReflectionResourceType* thisAsResourceType() const;
        const ShaderReflectionInterfaceType* thisAsInterfaceType() const;

        const std::shared_ptr<ShaderReflectionType>& getArrayElementType() const;
        uint32 getArrayElementCount() const;

        ptr_size getSizeInBytes() const { return mSize; }

        std::shared_ptr<ShaderReflectionVariable> getMember(const std::string& name) const;
        ShaderReflectionVariableOffsetAndType getMemberOffsetAndType(const std::string& name) const;
        ShaderReflectionVariableOffsetAndType getMemberOffsetAndTypeAt(ptr_size byteOffset) const;

        virtual bool operator==(const ShaderReflectionType& other) const = 0;
        bool operator!=(const ShaderReflectionType& other) { return !(*this == other); }

        struct DescriptorRange
        {
            EShaderResourceDescriptorType descriptorType;

            uint32 descriptorCount;

            // might be required for slangs ParameterBlocks
            uint32 baseIndex;
        };

        const DescriptorRange& getDescriptorRange(uint32 index) const { return mDescriptorRanges[index]; }

        uint32 getDescriptorRangeCount() const { return static_cast<uint32>(mDescriptorRanges.size()); }

        slang::TypeLayoutReflection* getSlangTypeLayoutReflection() const { return mSlangTypeLayoutReflection; }

    protected:
        ShaderReflectionType(EKind kind, ptr_size size, slang::TypeLayoutReflection* slangTypeLayoutReflection)
            : mKind(kind)
            , mSize(size)
            , mSlangTypeLayoutReflection(slangTypeLayoutReflection)
        {
        }

        EKind mKind;
        ptr_size mSize;

        std::vector<DescriptorRange> mDescriptorRanges;
        slang::TypeLayoutReflection* mSlangTypeLayoutReflection = nullptr;
    };

    class RAYCE_API_EXPORT ShaderReflectionArrayType : public ShaderReflectionType
    {
    public:
        ShaderReflectionArrayType(uint32 elementCount, uint32 elementStride, const std::shared_ptr<ShaderReflectionType>& elementReflectionType, uint32 size, slang::TypeLayoutReflection* slangTypeLayoutReflection);

        uint32 getElementCount() const { return mElements; }

        uint32 getElementStride() const { return mStridePerElement; }

        const std::shared_ptr<ShaderReflectionType>& getElementReflectionType() const { return mElementReflectionType; }

        bool operator==(const ShaderReflectionArrayType& other) const;
        bool operator==(const ShaderReflectionType& other) const override;

    private:
        uint32 mElements;
        uint32 mStridePerElement;
        std::shared_ptr<ShaderReflectionType> mElementReflectionType;
    };

    class RAYCE_API_EXPORT ShaderReflectionStructType : public ShaderReflectionType
    {
    public:
        ShaderReflectionStructType(const str& structName, uint32 size, slang::TypeLayoutReflection* slangTypeLayoutReflection);

        const str& getStructName() const { return mStructName; }

        uint32 getMemberCount() const { return static_cast<uint32>(mMemberVariables.size()); }

        const std::shared_ptr<ShaderReflectionVariable>& getMemberByIndex(uint32 index) const { return mMemberVariables[index]; }

        const std::shared_ptr<ShaderReflectionVariable>& getMemberByName(const str& name) const { return mMemberVariables[mMemberNameToIndex.at(name)]; }

        static constexpr int32 InvalidMemberIndex = -1;

        int32 indexOf(const str& name) const;

        ShaderReflectionVariableOffsetAndType getMemberOffsetAndTypeAt(ptr_size byteOffset) const;

        bool operator==(const ShaderReflectionStructType& other) const;
        bool operator==(const ShaderReflectionType& other) const override;

    private:
        str mStructName;
        std::vector<std::shared_ptr<ShaderReflectionVariable>> mMemberVariables;
        std::unordered_map<str, uint32> mMemberNameToIndex;
    };

    class RAYCE_API_EXPORT ShaderReflectionBasicType : public ShaderReflectionType
    {

    public:
        enum class EBasicType
        {
            Bool,
            Bool2,
            Bool3,
            Bool4,

            Uint8,
            Uint8_2,
            Uint8_3,
            Uint8_4,

            Uint16,
            Uint16_2,
            Uint16_3,
            Uint16_4,

            Uint,
            Uint2,
            Uint3,
            Uint4,

            Uint64,
            Uint64_2,
            Uint64_3,
            Uint64_4,

            Int8,
            Int8_2,
            Int8_3,
            Int8_4,

            Int16,
            Int16_2,
            Int16_3,
            Int16_4,

            Int,
            Int2,
            Int3,
            Int4,

            Int64,
            Int64_2,
            Int64_3,
            Int64_4,

            Float16,
            Float16_2,
            Float16_3,
            Float16_4,

            Float16_2x2,
            Float16_2x3,
            Float16_2x4,
            Float16_3x2,
            Float16_3x3,
            Float16_3x4,
            Float16_4x2,
            Float16_4x3,
            Float16_4x4,

            Float,
            Float2,
            Float3,
            Float4,

            Float2x2,
            Float2x3,
            Float2x4,
            Float3x2,
            Float3x3,
            Float3x4,
            Float4x2,
            Float4x3,
            Float4x4,

            Float64,
            Float64_2,
            Float64_3,
            Float64_4,

            Unknown = -1
        };

        ShaderReflectionBasicType(EBasicType basicType, bool rowMajor, uint32 size, slang::TypeLayoutReflection* slangTypeLayoutReflection);

        EBasicType getBasicType() const { return mType; }

        bool rowMajor() const { return mRowMajor; }

    private:
        EBasicType mType;
        bool mRowMajor;
    };

    class RAYCE_API_EXPORT ShaderReflectionResourceType : public ShaderReflectionType
    {
    public:
        enum class EAccess
        {
            Read,
            ReadWrite,

            Undefined
        };

        enum class EReturnType
        {
            Uint,
            Int,
            Float32,
            Float64,

            Unknown
        };

        enum class EDimensions
        {
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Texture1DArray,
            Texture2DArray,
            Texture1DMultisampleArray,
            Texture2DMultisampleArray,
            TextureCubeArray,

            Buffer,
            RaytracingAccelerationStructure,

            Count
        };

        enum class EStructuredBufferType
        {
            None,
            Default,
            Counter,
            Append,
            Consume
        };

        enum class EResourceType
        {
            Sampler,
            Texture,
            RawBuffer,
            TypedBuffer,
            StructuredBuffer,
            ConstantBuffer,
            CombinedTextureSampler,
            RaytracingAccelerationStructure
        };

        ShaderReflectionResourceType(EResourceType resourceType, EDimensions dimensions, EReturnType returnType, EAccess access, EStructuredBufferType structuredBufferType, slang::TypeLayoutReflection* slangTypeLayoutReflection);

        void setStructuredBufferReflectionType(const std::shared_ptr<ShaderReflectionType>& reflectionType);

        void setParameterBlockReflector(const std::shared_ptr<ShaderParameterBlockReflection>& parameterBlockReflection) { mParameterBlockReflection = parameterBlockReflection; }

        const std::shared_ptr<ShaderReflectionType>& getStructuredBufferReflectionType() const { return mStructuredBufferReflectionType; }

        const std::shared_ptr<ShaderParameterBlockReflection>& getParameterBlockReflection() const { return mParameterBlockReflection; }

        EDimensions getDimensions() const { return mDimensions; }

        EStructuredBufferType getStructuredBufferType() const { return mStructuredBufferType; }

        EReturnType getReturnType() const { return mReturnType; }

        EAccess getAccess() const { return mAccess; }

        EResourceType getResourceType() const { return mResourceType; }

        ptr_size getSizeInBytes() const { return mStructuredBufferReflectionType ? mStructuredBufferReflectionType->getSizeInBytes() : 0; }

        bool operator==(const ShaderReflectionResourceType& other) const;
        bool operator==(const ShaderReflectionType& other) const override;

    private:
        EResourceType mResourceType;
        EDimensions mDimensions;
        EReturnType mReturnType;
        EAccess mAccess;

        // Structured and Constant Buffers
        EStructuredBufferType mStructuredBufferType;
        std::shared_ptr<ShaderReflectionType> mStructuredBufferReflectionType;

        // Constant Buffers or ParameterBlocks
        std::shared_ptr<ShaderParameterBlockReflection> mParameterBlockReflection;
    };

    class RAYCE_API_EXPORT ShaderReflectionInterfaceType : public ShaderReflectionType
    {
    public:
        ShaderReflectionInterfaceType(slang::TypeLayoutReflection* slangTypeLayoutReflection);

        void setParameterBlockReflector(const std::shared_ptr<ShaderParameterBlockReflection>& parameterBlockReflection) { mParameterBlockReflection = parameterBlockReflection; }

        const std::shared_ptr<ShaderParameterBlockReflection>& getParameterBlockReflection() const { return mParameterBlockReflection; }

        bool operator==(const ShaderReflectionInterfaceType& other) const;
        bool operator==(const ShaderReflectionType& other) const override;

    private:
        // Spezialized interface types
        std::shared_ptr<ShaderParameterBlockReflection> mParameterBlockReflection;
    };

    class RAYCE_API_EXPORT ShaderReflectionVariable
    {
    public:
        ShaderReflectionVariable(const str& name, const std::shared_ptr<ShaderReflectionType> reflectionType, const ShaderReflectionVariableOffset& bindLocation);

        const str& getVariableName() const { return mName; }

        const std::shared_ptr<ShaderReflectionType>& getReflectionType() const { return mReflectionType; }

        const ShaderReflectionVariableOffset& getBindLocation() const { return mBindLocation; }

        ptr_size getBindOffset() const { return mBindLocation.getByteOffset(); }

        bool operator==(const ShaderReflectionVariable& other) const;
        bool operator!=(const ShaderReflectionVariable& other) const { return !(*this == other); }

    private:
        str mName;
        std::shared_ptr<ShaderReflectionType> mReflectionType;
        ShaderReflectionVariableOffset mBindLocation;
    };

    class RAYCE_API_EXPORT ShaderParameterBlockReflection
    {
    public:
        static constexpr uint32 InvalidIndex = 0xFFFFFFFF;

    private:
        std::shared_ptr<ShaderReflectionType> mElementType;

        DefaultConstantBufferBindingInfo mDefaultConstantBufferBindingInfo;

        std::vector<ResourceRangeBindingInfo> mResourceRanges;

        std::vector<DescriptorSetInfo> mDescriptorSets;

        bool mBuildDescriptorSets = false;

        std::vector<uint32> mRootDescriptorRangeIndices;

        std::vector<uint32> mParameterBlockSubObjectRangeIndices;
    };

    class RAYCE_API_EXPORT ShaderReflection
    {
    public:
        ShaderReflection(slang::ShaderReflection* slangTopLevelReflection, slang::EntryPointLayout* slangEntryPointLayout);
        ~ShaderReflection() = default;

    private:
        slang::ShaderReflection* mTopLevelReflection;
        slang::EntryPointLayout* mEntryPointLayout;

        std::vector<std::pair<uint32, VkDescriptorSetLayoutBinding>> mDescriptorSetLayoutBindings = {};
        std::vector<VkPushConstantRange> mPushConstantRanges                                      = {}; // push constants available at this stage
        VkVertexInputBindingDescription mVertexInputBinding                                       = {};
        std::vector<VkVertexInputAttributeDescription> mVertexInputAttributes                     = {};
        // work packages for compute
        uint32 mLocalSizeX = 0, mLocalSizeY = 0, mLocalSizeZ = 0;
    };
} // namespace rayce

#endif // SHADER_REFLECTION_HPP
