__exported import core.hostDeviceInterop;

static const float INFINITY = 1e32;
static const float EPSILON = 1e-6;

static const float PI = 3.14159265358979323846;
static const float TWO_PI = (2.0 * PI);
static const float FOUR_PI = (4.0 * PI);
static const float INV_PI = 0.31830988618379067154;
static const float INV_TWO_PI = 0.15915494309189533577;
static const float INV_FOUR_PI = 0.07957747154594766788;
static const float HALF_PI = 1.57079632679489661923;
static const float QUARTER_PI = 0.78539816339744830961;
static const float SQRT2 = 1.41421356237309504880;

struct DeviceTriangle
{
    Vertex vertices[3];
    float3 barycentrics;
    float2 interpolatedUV;
    float3 geometryNormal;
    float3 interpolatedNormal;

    float3 dfd1;
    float3 dfd2;
    float2 uvd1;
    float2 uvd2;

    uint materialId;
    int lightId;
};

struct DeviceSphere
{
    float3 normal;
    uint materialId;

    float2 uv;
    int lightId;
};

enum EHitKind : uint8_t
{
    miss = 0,
    triangleMesh = 1,
    proceduralSphere = 2
};

struct CoordinateSpace
{
    __init(const float3 normal, const bool sphere = false)
    {
        this.normal = normal;

        if(sphere)
        {
            // two ugly points but kind of consistent with uvs ...
            this.tangent = normalize(float3(-normal.z, 0.0, normal.x));
            this.bitangent = cross(normal, this.tangent);
        }
        else
        {
            // Not consistent for spheres
            // Building an Orthonormal Basis, Revisited, (JCGT), vol. 6, no. 1, 1-8, 2017
            // Available online: http://jcgt.org/published/0006/01/01/
            const float normalSign = normal.z < 0.0 ? -1.0 : 1.0;
            const float a = -rcp(normalSign + normal.z);
            const float b = normal.x * normal.y * a;
            this.tangent = float3(1.0 + normalSign * normal.x * normal.x * a, normalSign * b, -normalSign * normal.x);
            this.bitangent = float3(b, normalSign + normal.y * normal.y * a, -normal.y);}
    }

    __init(const float3 normal, bool canUsePartials, const float3 dfd1, const float3 dfd2, const float2 uvd1, const float2 uvd2)
    {
        this.normal = normal;
        if(canUsePartials && uvd1.x > 0.0)
        {
            const float3 t_    = (uvd2.y * dfd1 - uvd1.y * dfd2) / (uvd1.x * uvd2.y - uvd2.x * uvd1.y);
            this.tangent    = normalize(t_ - normal * dot(normal, t_));
            this.bitangent  = cross(normal, this.tangent);
            return this;
        }
        else
        {
            return CoordinateSpace(normal);
        }
    }

    __init(const float3 normal, const float3 tangent, const float3 bitangent)
    {
        this.normal = normal;
        this.tangent = tangent;
        this.bitangent = bitangent;
    }

    CoordinateSpace flipped()
    {
        CoordinateSpace flippedSpace;
        flippedSpace.normal = -this.normal;
        flippedSpace.tangent = -this.tangent;
        flippedSpace.bitangent = -this.bitangent;
        return flippedSpace;
    }

    float3 tangentToWorld(const float3 p)
    {
        return p.x * tangent + (p.y * bitangent + p.z * normal);
    }

    float3 worldToTangentFrame(const float3 p)
    {
        return float3(dot(p, tangent), dot(p, bitangent), dot(p, normal));
    }

    float3 normal;
    float3 tangent;
    float3 bitangent;
};

// FIXME: We should reduce the size - what about packing?
[payload]
struct RayPayload
{
    float3 hitPoint;
    EHitKind hitKind;

    float2 uv;
    uint materialId;
    int lightId;

    CoordinateSpace space;
};

[payload]
struct ShadowRayPayload
{
    bool hit;
};

enum EBxDFFlags : uint
{
    unset                = 0,

    reflection           = 1 << 0,
    transmission         = 1 << 1,

    diffuse              = 1 << 2,
    glossy               = 1 << 3,
    specular             = 1 << 4,

    diffuseReflection    = (uint)EBxDFFlags::diffuse | (uint)EBxDFFlags::reflection,
    diffuseTransmission  = (uint)EBxDFFlags::diffuse | (uint)EBxDFFlags::transmission,

    glossyReflection     = (uint)EBxDFFlags::glossy | (uint)EBxDFFlags::reflection,
    glossyTransmission   = (uint)EBxDFFlags::glossy | (uint)EBxDFFlags::transmission,

    specularReflection   = (uint)EBxDFFlags::specular | (uint)EBxDFFlags::reflection,
    specularTransmission = (uint)EBxDFFlags::specular | (uint)EBxDFFlags::transmission,

    all                  = (uint)EBxDFFlags::diffuse | (uint)EBxDFFlags::glossy | (uint)EBxDFFlags::specular |
                            (uint)EBxDFFlags::reflection | (uint)EBxDFFlags::transmission
};

bool isReflective(EBxDFFlags f) { return ((uint)f & (uint)EBxDFFlags::reflection) != 0; }
bool isTransmissive(EBxDFFlags f) { return ((uint)f & (uint)EBxDFFlags::transmission) != 0; }
bool isDiffuse(EBxDFFlags f) { return ((uint)f & (uint)EBxDFFlags::diffuse) != 0; }
bool isGlossy(EBxDFFlags f) { return ((uint)f & (uint)EBxDFFlags::glossy) != 0; }
bool isSpecular(EBxDFFlags f) { return ((uint)f & (uint)EBxDFFlags::specular) != 0; }
bool isNonSpecular(EBxDFFlags f) { return ((uint)f & ((uint)EBxDFFlags::diffuse | (uint)EBxDFFlags::glossy)) != 0; }

enum EBxDFSampleFlags  : uint
{
    unset        = 0,

    reflection   = 1 << 0,
    transmission = 1 << 1,

    all          = (uint)EBxDFFlags::reflection | (uint)EBxDFFlags::transmission
};

bool sampleReflection(EBxDFSampleFlags f) { return ((uint)f & (uint)EBxDFSampleFlags::reflection) != 0; }
bool sampleTransmission(EBxDFSampleFlags f) { return ((uint)f & (uint)EBxDFSampleFlags::transmission) != 0; }

struct BxDFSample
{
    float3 f;
    float eta = 1.0;
    float3 wi;
    float pdf = 0.0;
    EBxDFFlags flags = EBxDFFlags::unset;

    bool proportionalPDF = false;

    bool isReflection() { return isReflective(flags); }
    bool isTransmission() { return isTransmissive(flags); }
    bool isDiffuse() { return isDiffuse(flags); }
    bool isGlossy() { return isGlossy(flags); }
    bool isSpecular() { return isSpecular(flags); }
};

struct MicrofacetSample
{
    float3 wm;
    float pdf;
};

struct LightSample
{
    float3 radiance;
    float pdf = 0.0;
    float3 wi;
};

struct VertexLightSample
{
    float3 radiance;
    float pdf = 0.0;
    float3 wi;
    float cosTheta;
    float3 point;
    float pdfDirection;
    float3 normal;
    float pad;
};

struct SphereIntersectionAttributes {};

struct Ray
{
    float3 origin;
    float3 direction;

    __init()
    {
        this.origin = float3(0.0);
        this.direction = float3(0.0);
    }

    float3 offsetOrigin()
    {
        return origin + direction * EPSILON;
    }
};
