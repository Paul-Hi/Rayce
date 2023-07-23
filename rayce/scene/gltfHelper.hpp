/// @file      gltfHelper.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef GLTF_HELPER_HPP
#define GLTF_HELPER_HPP

namespace rayce
{
    template <typename ComponentType, uint32 componentCount>
    static std::vector<vec3> convertGltfDataVec3(
        const byte* adress, const uint32 count, const uint32 byteStride, std::function<ComponentType(const ComponentType&)> apply = [](const ComponentType& entry)
                                                                         { return entry; })
    {
        std::vector<vec3> result;

        static_assert(componentCount == 3, "Can not convert to vec3!");

        auto current = adress;

        uint32 toTypeCount = count / componentCount;

        for (ptr_size i = 0; i < toTypeCount; ++i)
        {
            vec3 p;
            for (ptr_size c = 0; c < componentCount; ++c)
            {
                p.x() = static_cast<float>(apply(*reinterpret_cast<const ComponentType*>(current + 0 * sizeof(ComponentType))));
                p.y() = static_cast<float>(apply(*reinterpret_cast<const ComponentType*>(current + 1 * sizeof(ComponentType))));
                p.z() = static_cast<float>(apply(*reinterpret_cast<const ComponentType*>(current + 2 * sizeof(ComponentType))));

                current += byteStride;
            }

            result.push_back(p);
        }

        return result;
    }

    template <typename ComponentType, uint32 componentCount>
    static std::vector<vec2> convertGltfDataVec2(
        const byte* adress, const uint32 count, const uint32 byteStride, std::function<ComponentType(const ComponentType&)> apply = [](const ComponentType& entry)
                                                                         { return entry; })
    {
        std::vector<vec2> result;

        static_assert(componentCount == 2, "Can not convert to vec2!");

        auto current = adress;

        uint32 toTypeCount = count / componentCount;

        for (ptr_size i = 0; i < toTypeCount; ++i)
        {
            vec2 p;
            for (ptr_size c = 0; c < componentCount; ++c)
            {
                p.x() = static_cast<float>(apply(*reinterpret_cast<const ComponentType*>(current + 0 * sizeof(ComponentType))));
                p.y() = static_cast<float>(apply(*reinterpret_cast<const ComponentType*>(current + 1 * sizeof(ComponentType))));

                current += byteStride;
            }

            result.push_back(p);
        }

        return result;
    }

    template <typename ComponentType, uint32 componentCount>
    static std::vector<uint32> convertGltfDataUint32(
        const byte* adress, const uint32 count, const uint32 byteStride, std::function<ComponentType(const ComponentType&)> apply = [](const ComponentType& entry)
                                                                         { return entry; })
    {
        std::vector<uint32> result;

        static_assert(componentCount == 1, "Can not convert to uint32!");

        auto current = adress;

        uint32 toTypeCount = count;

        for (ptr_size i = 0; i < toTypeCount; ++i)
        {
            uint32 p = static_cast<uint32>(apply(*reinterpret_cast<const ComponentType*>(current)));

            current += byteStride;

            result.push_back(p);
        }

        return result;
    }

    template <typename T>
    static void extendVector(std::vector<T>& toExtend, const std::vector<T>& with)
    {
        toExtend.reserve(toExtend.size() + with.size());
        toExtend.insert(toExtend.end(), with.begin(), with.end());
    }

    static VkFormat getImageFormat(uint32 components, bool srgb)
    {
        // FIXME: This is really basic...
        if (srgb)
        {
            // clang-format off
            return components == 1 ? VK_FORMAT_R8_SRGB :
                    (components == 2 ? VK_FORMAT_R8G8_SRGB :
                    (components == 3 ? VK_FORMAT_R8G8B8_SRGB :
                    VK_FORMAT_R8G8B8A8_SRGB));
            // clang-format on
        }
        else
        {
            // clang-format off
            return components == 1 ? VK_FORMAT_R8_UNORM :
                    (components == 2 ? VK_FORMAT_R8G8_UNORM :
                    (components == 3 ? VK_FORMAT_R8G8B8_UNORM :
                    VK_FORMAT_R8G8B8A8_UNORM));
            // clang-format on
        }
    }
}

#endif // GLTF_HELPER_HPP
