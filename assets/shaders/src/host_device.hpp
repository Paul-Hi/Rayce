#ifndef HOST_DEVICE_HPP
#define HOST_DEVICE_HPP

#ifdef __cplusplus
#include <core/types.hpp>
using uint     = rayce::uint32;
using uint64_t = rayce::uint64;
// clang-format off
#define START_BINDING(a) enum a {
#define END_BINDING() }
// clang-format on
namespace rayce
{
#else
#define RAYCE_API_EXPORT
#define START_BINDING(a) const uint
#define END_BINDING()
#endif

    struct RAYCE_API_EXPORT Vertex
    {
        vec3 position;
        vec3 normal;
        vec2 uv;

#ifdef __cplusplus
        static VkVertexInputBindingDescription getVertexInputBindingDescription()
        {
            VkVertexInputBindingDescription vertexInputBindingDescription{};
            vertexInputBindingDescription.binding   = 0;
            vertexInputBindingDescription.stride    = sizeof(Vertex);
            vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vertexInputBindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getVertexInputAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributeDescription{};
            vertexInputAttributeDescription[0].binding  = 0;
            vertexInputAttributeDescription[0].location = 0;
            vertexInputAttributeDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vertexInputAttributeDescription[0].offset   = offsetof(Vertex, position);

            vertexInputAttributeDescription[1].binding  = 0;
            vertexInputAttributeDescription[1].location = 1;
            vertexInputAttributeDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vertexInputAttributeDescription[1].offset   = offsetof(Vertex, normal);

            vertexInputAttributeDescription[2].binding  = 0;
            vertexInputAttributeDescription[2].location = 2;
            vertexInputAttributeDescription[2].format   = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributeDescription[2].offset   = offsetof(Vertex, uv);

            return vertexInputAttributeDescription;
        }

        static ptr_size getSize()
        {
            return sizeof(Vertex);
        }
#endif
    };

    struct RAYCE_API_EXPORT InstanceData
    {
        uint64_t indexReference;
        uint64_t vertexReference;
        uint materialId;

        int pad[3];
    };

    struct RAYCE_API_EXPORT Material
    {
        vec4 baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        vec3 emissiveColor = vec3(0.0f, 0.0f, 0.0f);
        // alphaMode;
        // alphaCutoff;
        // doubleSided;
        float metallicFactor = 0.5f;
        float roughnessFactor = 0.5f;

        int baseColorTextureId = -1;
        int metallicRoughnessTextureId = -1;
        int normalTextureId = -1;
        // int32 occlusionTextureId = -1;
        int emissiveTextureId = -1;

        int pad[3];
    };

#ifdef __cplusplus
}
#endif

#endif // HOST_DEVICE_HPP