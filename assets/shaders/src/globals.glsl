#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#include "host_device.hpp"

#define INFINITY 1e32
#define EPSILON 1e-6

#define PI 3.1415926535897932384626433832795
#define INV_PI (1.0 / PI)
#define TWO_PI (2.0 * PI)
#define HALF_PI (0.5 * PI)

struct Tri
{
    Vertex vertices[3];
    vec3 barycentrics;
    vec2 interpolatedUV;
    vec3 geometryNormal;
    vec3 interpolatedNormal;

    vec3 dfd1;
    vec3 dfd2;
    vec2 uvd1;
    vec2 uvd2;

    uint materialId;
    int lightId;
};

struct Sph
{
    vec3 normal;

    uint materialId;
    int lightId;
};

uint hitKindTriangleMesh = 0;
uint hitKindProceduralSphere = 1;

struct Hit
{
    uint hitKind;
    uint primitiveId;
    uint instanceCustomIndex;
    vec2 hitAttributes;
    mat4x3 worldToObject;
    mat4x3 objectToWorld;
};

struct RayPayload
{
    bool hit;
    vec3 hitPoint;
    Hit hitInfo;
};

struct BSDFSample
{
    vec3 reflectance;
    float pdf;
    bool dirac;
};

struct LightSample
{
    vec3 radiance;
    float pdf;
};

#endif  // GLOBALS_GLSL