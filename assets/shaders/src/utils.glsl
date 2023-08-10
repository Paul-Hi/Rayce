#ifndef UTILS_GLSL
#define UTILS_GLSL

void createTBN(in vec3 normal,
                in vec3 dfd1, in vec3 dfd2,
                in vec2 uvd1, in vec2 uvd2,
                out vec3 tangent, out vec3 bitangent)
{
    vec3 t_    = (uvd2.y * dfd1 - uvd1.y * dfd2) / (uvd1.x * uvd2.y - uvd2.x * uvd1.y);
    tangent    = normalize(t_ - normal * dot(normal, t_));
    bitangent  = cross(normal, tangent);
}

const float GAMMA     = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;
vec3 linearTosRGB(vec3 color)
{
    return pow(color, vec3(INV_GAMMA));
}

// ACES tone map
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES(vec3 color)
{
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return linearTosRGB(clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0));
}


#endif  // UTILS_GLSL