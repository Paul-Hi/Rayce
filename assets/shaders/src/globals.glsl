#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#include "host_device.hpp"

#define INFINITY 1e32

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
    float hitT;
    Tri triangle;
};

#endif  // GLOBALS_GLSL