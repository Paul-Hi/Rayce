__exported import core.hostDeviceInterop;

static const float INFINITY = 1e32;
static const float EPSILON = 1e-6;

static const float PI = 3.14159265358979323846;
static const float TWO_PI = (2.0 * PI);
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

// FIXME: We should reduce the size - what about packing?

[payload]
struct RayPayload
{
    float3 hitPoint;
    EHitKind hitKind;

    float2 hitAttributes;
    uint primitiveId;
    uint instanceCustomIndex;

    float4x3 worldToObject;
    // float4x3 objectToWorld;
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
    bool valid = false;
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
