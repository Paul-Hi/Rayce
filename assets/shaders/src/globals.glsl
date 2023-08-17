#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#include "host_device.hpp"

#define INFINITY 1e32
#define EPSILON 1e-9

#define PI 3.1415926535897932384626433832795
#define INV_PI (1.0 / PI)
#define TWO_PI (2.0 * PI)
#define HALF_PI (0.5 * PI)

struct Tri
{
    Vertex vertices[3];
    vec3 barycentrics;
    vec2 interpolatedUV;
    vec3 interpolatedNormal;

    vec3 dfd1;
    vec3 dfd2;
    vec2 uvd1;
    vec2 uvd2;

    uint materialId;
};

struct RayPayload
{
    bool hit;
    vec3 hitPoint;
    Tri triangle;
};

struct BSDFSample
{
    vec3 reflectance;
    float pdf;
};

#endif  // GLOBALS_GLSL