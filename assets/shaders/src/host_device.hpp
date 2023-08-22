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

// binding points
#define RT_SET 0
#define TLAS_BINDING 0
#define ACCUM_BINDING 1
#define RESULT_BINDING 2

#define CAMERA_SET 1
#define CAMERA_BINDING 0

#define MODEL_SET 2
#define TEXTURE_BINDING 0
#define INSTANCE_BINDING 1
#define MATERIAL_BINDING 2
#define LIGHT_BINDING 3
#define SPHERE_BINDING 4

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

    struct RAYCE_API_EXPORT AxisAlignedBoundingBox
    {
        vec3 minimum;
        vec3 maximum;

        int pad[2];
    };

    struct RAYCE_API_EXPORT Sphere
    {
        vec3 center;
        float radius;
    };

    struct RAYCE_API_EXPORT InstanceData
    {
        uint64_t indexReference;
        uint64_t vertexReference;
        uint materialId;
        int lightId;
        int sphereId;

        int pad;
    };

// clang-format off
#ifdef __cplusplus
#define ENUM(a)                          \
  enum class RAYCE_API_EXPORT a : uint \
  {
#define ENUM_END() }
#else
#define ENUM(a) const uint
#define ENUM_END()
#define ELightType uint
#define EBSDFType uint
#endif

    ENUM(EShapeType)
        triangleMesh = 0,
        sphere = 1
    ENUM_END();

    ENUM(ELightType)
        area = 0,
        point = 1,
        constant = 2,
        envmap = 3,
        spot = 4,
        projector = 5,
        directional = 6,
        directionalArea = 7,
        lightTypeCount = 8
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
        bsdfTypeCount = 8
    ENUM_END();

    struct RAYCE_API_EXPORT Light
    {
        ELightType type;

        //  area light
        uint sphereId;
        int radianceTexture;
        int pad0;
        vec3 radiance;
        int pad1;

#ifdef __cplusplus
        Light()
        : type(ELightType::area)
        , sphereId(0)
        , radianceTexture(-1)
        , radiance(vec3(0.0, 0.0, 0.0))
        {}
#endif
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
        int alphaTexture;

        // for complex conducting materials
        int complexIorTexture;
        vec2 complexIor; // eta, k

        uint nonlinear;

        uint canUseUv;

#ifdef __cplusplus
        Material()
        : diffuseReflectance(vec3(0.5, 0.5, 0.5))
        , diffuseReflectanceTexture(-1)
        , twoSided(0)
        , bsdfType(EBSDFType::diffuse)
        , interiorIor(1.5046f)
        , exteriorIor(1.000277f)
        , specularReflectance(vec3(1.0, 1.0, 1.0))
        , specularReflectanceTexture(-1)
        , specularTransmittance(vec3(1.0, 1.0, 1.0))
        , specularTransmittanceTexture(-1)
        , alpha(vec2(0.1, 0.1))
        , alphaTexture(-1)
        , complexIorTexture(-1)
        , complexIor(vec2(0.0, 1.0))
        , nonlinear(0)
        , canUseUv(0)
        {}
#endif

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
