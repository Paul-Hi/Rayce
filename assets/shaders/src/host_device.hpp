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

// clang-format off
#ifdef __cplusplus
#define ENUM(a)                 \
  enum class RAYCE_API_EXPORT a \
  {
#define ENUM_END() }
#else
#define ENUM(a) const uint
#define ENUM_END()
#endif

    ENUM(ELightType)
        area = 0,
        point = 1,
        constant = 2,
        envmap = 3,
        spot = 4,
        projector = 5,
        directional = 6,
        directionalArea = 7,
        count = 8
    ENUM_END();

    ENUM(EBSDFType)
        diffuse = 0,
        smoothDielectric = 1,
        smoothDielectricThin = 2,
        roughDielectric = 3,
        smoothConductor = 4,
        roughConductor = 5,
        smoothPlastic = 6,
        roughPlastic = 7,
        count = 8
    ENUM_END();

    struct RAYCE_API_EXPORT Light
    {
        ELightType type;

        //  area light
        uint primitiveId;
        vec3 radiance;
        int radianceTexture;
    };

    struct RAYCE_API_EXPORT Material
    {
        vec3 diffuseReflectance;
        int diffuseReflectanceTexture;

        uint twoSided;
        EBSDFType bsdfType;

        float interiorIor;
        float exteriorIor;

        vec3 specularReflectance; // for physical realism, do not touch - default 1
        int specularReflectanceTexture;
        vec3 specularTransmittance; // for physical realism, do not touch - default 1
        int specularTransmittanceTexture;

        vec2 alpha; // alpha u, alpha v;
        int alphTexture;

        // for complex conducting materials
        vec2 complexIor; // eta, k
        int complexIorTexture;

        uint nonlinear;

        uint canUseUv;

        /*
        // Disney (later)
        vec3 baseColor;
        float metallic;       // 0.0 - 1.0
        float subsurface;     // 0.0 - 1.0
        float specular;       // 0.0 - 1.0
        float roughness;      // 0.0 - 1.0
        float specularTint;   // 0.0 - 1.0
        float anisotropic;    // 0.0 - 1.0
        float sheen;          // 0.0 - 1.0
        float sheenTint;      // 0.0 - 1.0
        float clearcoat;      // 0.0 - 1.0
        float clearcoatGloss; // 0.0 - 1.0

        // bsdf
        float specTrans; // 0.0 - 1.0
        float ior;
        vec3 scatrDist;

        float flatness;  // 0.0 - 1.0
        float diffTrans; // 0.0 - 1.0

        // not official?
        vec3 emission;
        */
    };

#ifdef __cplusplus
}
#endif

#endif // HOST_DEVICE_HPP