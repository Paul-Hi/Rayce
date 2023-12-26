#ifndef HOST_DEVICE_HPP
#define HOST_DEVICE_HPP

#ifdef __cplusplus
#include <core/types.hpp>
using uint     = rayce::uint32;
using uint64_t = rayce::uint64;
using float3   = rayce::vec3;
using float4   = rayce::vec4;
using float2   = rayce::vec2;
using int2     = rayce::ivec2;
using float4x4 = rayce::mat4;
namespace rayce
{
#else
#define RAYCE_API_EXPORT
#endif

// binding points
    static const int VERTEX_SET     = 0;
    static const int VERTEX_BINDING = 0;

    static const int INDEX_SET     = 1;
    static const int INDEX_BINDING = 0;

    static const int RT_SET         = 2;
    static const int TLAS_BINDING   = 0;
    static const int ACCUM_BINDING  = 1;
    static const int RESULT_BINDING = 2;

    static const int CAMERA_SET     = 3;
    static const int CAMERA_BINDING = 0;

    static const int MODEL_SET        = 4;
    static const int TEXTURE_BINDING  = 0; // Combined Image Sampler
    // static const int SAMPLER_BINDING  = 0; // Combined Image Sampler
    static const int INSTANCE_BINDING = 1;
    static const int MATERIAL_BINDING = 2;
    static const int LIGHT_BINDING    = 3;
    static const int SPHERE_BINDING   = 4;

    enum RAYCE_API_EXPORT EIntegratorType : uint
    {
        direct           = 0,
        path             = 1,
        debugDepth       = 2,
        debugNormals     = 3,
        debugReflectance = 4,
        debugEmission    = 5,
    };

    struct RAYCE_API_EXPORT Vertex
    {
        float3 position;
        uint pad0 = 0;
        float3 normal;
        uint pad1 = 0;
        float2 uv;
        int2 pad2;

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
        float3 minimum;
        float3 maximum;
    };

    struct RAYCE_API_EXPORT Sphere
    {
        float3 center;
        float radius;
    };

    struct RAYCE_API_EXPORT InstanceData
    {
        int objectIndex;
        uint materialId;
        int lightId;
        int sphereId;
    };

    enum RAYCE_API_EXPORT EShapeType : uint
    {
        triangleMesh = 0,
        sphere       = 1,
        rectangle    = 2,
    };

    enum RAYCE_API_EXPORT ELightType : uint
    {
        area              = 0,
        analyticSphere    = 1,
        analyticRectangle = 2,
        // point           = 2,
        // constant        = 3,
        // envmap          = 4,
        // spot            = 5,
        // projector       = 6,
        // directional     = 7,
        // directionalArea = 8,
        lightTypeCount = 3
    };

    enum RAYCE_API_EXPORT EBxDFType : uint
    {
        diffuse              = 0,
        smoothDielectric     = 1,
        smoothDielectricThin = 2,
        roughDielectric      = 3,
        smoothConductor      = 4,
        roughConductor       = 5,
        smoothPlastic        = 6,
        roughPlastic         = 7,
        bsdfTypeCount        = 8
    };

    struct RAYCE_API_EXPORT Light
    {
        float3     wCenter;
        ELightType type;

        float3 radiance;
        int    radianceTexture;

        // float3   direction;
        // float    openingAngle;
        // float    cosOpeningAngle;
        // float    cosSubtendedAngle;
        // float    penumbraAngle;

        // analytic area lights
        float3   tangent;
        float    surfaceArea;
        float3   bitangent;
        float    pad0;
        float4x4 lightToWorld;
        float4x4 worldToLight;


#ifdef __cplusplus
        Light()
            : wCenter(float3::Zero())
            , type(ELightType::analyticSphere)
            , radiance(float3::Zero())
            , radianceTexture(-1)
            , tangent(float3::Zero())
            , surfaceArea(0.0)
            , bitangent(float3::Zero())
            , pad0(0)
            , lightToWorld(float4x4::Zero())
            , worldToLight(float4x4::Zero())
        {
        }
#else
        __init()
        {
            this.wCenter = float3(0.0);
            this.type = ELightType::analyticSphere;
            this.radiance = float3(0.0, 0.0, 0.0);
            this.radianceTexture = -1;
            this.tangent = float3(0.0, 0.0, 0.0);
            this.surfaceArea = 0.0;
            this.bitangent = float3(0.0, 0.0, 0.0);
            this.pad0 = 0;
            this.lightToWorld = float4x4(0.0);
            this.worldToLight = float4x4(0.0);
        }
#endif
    };

    struct RAYCE_API_EXPORT CameraDataRT
    {
        float4x4 inverseView;
        float4x4 inverseProjection;
        float4 pbData; // lens radius, focal distance, near, far
    };

    struct RAYCE_API_EXPORT Material
    {
        float3 diffuseReflectance;
        int diffuseReflectanceTexture;

        uint twoSided;
        EBxDFType bxdfType;

        float interiorIor;
        float exteriorIor;

        float3 specularReflectance; // for physical realism, do not touch - default 1
        int specularReflectanceTexture;
        float3 specularTransmittance; // for physical realism, do not touch - default 1
        int specularTransmittanceTexture;

        float3 alpha; // alpha u, alpha v, unused
        int alphaTexture;

        // for complex conducting materials
        float3 conductorEta;
        int conductorEtaTexture;
        float3 conductorK;
        int conductorKTexture;

        uint nonlinear;

        uint canUseUv;

        float2 pad;

#ifdef __cplusplus
        Material()
            : diffuseReflectance(float3(0.5, 0.5, 0.5))
            , diffuseReflectanceTexture(-1)
            , twoSided(0)
            , bxdfType(EBxDFType::diffuse)
            , interiorIor(1.5046f)
            , exteriorIor(1.000277f)
            , specularReflectance(float3(1.0, 1.0, 1.0))
            , specularReflectanceTexture(-1)
            , specularTransmittance(float3(1.0, 1.0, 1.0))
            , specularTransmittanceTexture(-1)
            , alpha(float3(0.1, 0.1, 0.0))
            , alphaTexture(-1)
            , conductorEta(float3(0.0, 0.0, 0.0))
            , conductorEtaTexture(-1)
            , conductorK(float3(1.0, 1.0, 1.0))
            , conductorKTexture(-1)
            , nonlinear(0)
            , canUseUv(0)
        {
        }
#else
        __init()
        {
            this.diffuseReflectance = float3(0.5, 0.5, 0.5);
            this.diffuseReflectanceTexture = -1;
            this.twoSided = 0;
            this.bxdfType = EBxDFType::diffuse;
            this.interiorIor = 1.5046f;
            this.exteriorIor = 1.000277f;
            this.specularReflectance = float3(1.0, 1.0, 1.0);
            this.specularReflectanceTexture = -1;
            this.specularTransmittance = float3(1.0, 1.0, 1.0);
            this.specularTransmittanceTexture = -1;
            this.alpha = float3(0.1, 0.1, 0.0);
            this.alphaTexture = -1;
            this.conductorEta = float3(0.0, 0.0, 0.0);
            this.conductorEtaTexture = -1;
            this.conductorK = float3(1.0, 1.0, 1.0);
            this.conductorKTexture = -1;
            this.nonlinear = 0;
            this.canUseUv = 0;
        }
#endif

        /*
        // Disney (later)
        float3 baseColor;
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
        float3 scatrDist;

        float flatness;  // 0.0 - 1.0
        float diffTrans; // 0.0 - 1.0

        // not official?
        float3 emission;
        */
    };

#ifdef __cplusplus
}
#endif

#endif // HOST_DEVICE_HPP
