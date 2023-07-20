#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#define INFINITY 1e32

struct Vertex
{
    vec3 position;
    vec2 uv;
};

struct Tri
{
    Vertex vertices[3];
    vec3 barycentrics;
    vec2 interpolatedUV;
};

struct RayPayload
{
    float hitT;
    Tri triangle;
};

#endif  // GLOBALS_GLSL