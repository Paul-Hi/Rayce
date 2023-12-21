__exported import hostDeviceInterop;

static const float INFINITY = 1e32;
static const float EPSILON = 1e-6;

static const float PI = 3.1415926535897932384626433832795;
static const float INV_PI = (1.0 / PI);
static const float TWO_PI = (2.0 * PI);
static const float HALF_PI = (0.5 * PI);
static const float QUARTER_PI = (0.25 * PI);

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

enum EHitKind : uint
{
    triangleMesh = 0,
    proceduralSphere = 1
};

struct Hit
{
    EHitKind hitKind;
    uint primitiveId;
    uint instanceCustomIndex;
    float2 hitAttributes;
    float4x3 worldToObject;
    float4x3 objectToWorld;
};

struct RayPayload
{
    bool hit;
    float3 hitPoint;
    Hit hitInfo;
};

struct BxDFSample
{
    float3 reflectance;
    float3 wi;
    float pdf;
    bool dirac;
};

struct MicrofacetSample
{
    float3 wm;
    float pdf;
};

struct LightSample
{
    float3 radiance;
    float3 wi;
    float pdf;
    bool valid;
};

struct SphereIntersectionAttributes
{
    bool inside;
};

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
