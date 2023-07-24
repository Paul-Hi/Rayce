#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#define INFINITY 1e32

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct InstanceData
{
    uint64_t indexReference;
    uint64_t vertexReference;
    uint materialId;
};

struct Material
{
    vec4 baseColor;
    vec3 emissiveColor;
    float metallicFactor;
    float roughnessFactor;

    int baseColorTextureId;
    int metallicRoughnessTextureId;
    int normalTextureId;
    int emissiveTextureId;

    int pad[3];
};

struct Tri
{
    Vertex vertices[3];
    vec3 barycentrics;
    vec2 interpolatedUV;
    uint materialId;
};

struct RayPayload
{
    float hitT;
    Tri triangle;
};

#endif  // GLOBALS_GLSL