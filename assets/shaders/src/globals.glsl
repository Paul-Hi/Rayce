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
    uint materialId;
};

struct RayPayload
{
    float hitT;
    Tri triangle;
};

#endif  // GLOBALS_GLSL