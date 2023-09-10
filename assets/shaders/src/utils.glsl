#ifndef UTILS_GLSL
#define UTILS_GLSL

float getLuminance(in vec3 baseColor)
{
    // approximation
    return dot(vec3(0.3, 0.6, 1.0), baseColor);
}

void createCoordinateSystem(in vec3 normal, out vec3 tangent, out vec3 bitangent,
                            in bool hasUV,
                            in vec3 dfd1, in vec3 dfd2,
                            in vec2 uvd1, in vec2 uvd2
                            )
{
    if(hasUV && uvd1.x > 0.0)
    {
        vec3 t_    = (uvd2.y * dfd1 - uvd1.y * dfd2) / (uvd1.x * uvd2.y - uvd2.x * uvd1.y);
        tangent    = normalize(t_ - normal * dot(normal, t_));
        bitangent  = cross(normal, tangent);
    }
    else
    {
        // Building an Orthonormal Basis, Revisited, (JCGT), vol. 6, no. 1, 1-8, 2017
        // Available online: http://jcgt.org/published/0006/01/01/
        float normalSign = normal.z < 0.0 ? -1.0 : 1.0;
        float a = -1.0 / (normalSign + normal.z);
        float b = normal.x * normal.y * a;
        tangent = vec3(1.0 + normalSign * normal.x * normal.x * a, normalSign * b, -normalSign * normal.x);
        bitangent = vec3(b, normalSign + normal.y * normal.y * a, -normal.y);
    }
}

void createCoordinateSystem(in vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    createCoordinateSystem(normal, tangent, bitangent,
                           false,
                           vec3(0.0), vec3(0.0),
                           vec2(0.0), vec2(0.0)
                           );
}

// beta = 2 - number samples = 1
float powerHeuristic(in float fPdf, in float gPdf) {
    fPdf *= fPdf;
    gPdf *= gPdf;
    return fPdf / (fPdf + gPdf);
}

vec3 tangentToWorld(in vec3 p, in vec3 tangent, in vec3 bitangent, in vec3 normal)
{
    return fma(vec3(p.x), tangent, fma(vec3(p.y), bitangent, p.z * normal));
}

vec3 worldToTangentFrame(in vec3 p, in vec3 tangent, in vec3 bitangent, in vec3 normal)
{
    return vec3(dot(p, tangent), dot(p, bitangent), dot(p, normal));
}

bool hasDiracPdf(in Material material)
{
    if(material.bsdfType == smoothDielectric ||
        material.bsdfType == smoothDielectricThin ||
        material.bsdfType == smoothConductor ||
        material.bsdfType == smoothPlastic)
    {
        return true;
    }
    return false;
}

// tangent space trigonometry
float cosTheta(in const vec3 w) { return w.z; }
float cos2Theta(in const vec3 w) { return w.z * w.z; }
float sin2Theta(in const vec3 w) { return max(0.0, 1.0 - cos2Theta(w)); }
float sinTheta(in const vec3 w) { return sqrt(sin2Theta(w)); }
float tanTheta(in const vec3 w) { return sinTheta(w) / cosTheta(w); }
float tan2Theta(in const vec3 w) { return sin2Theta(w) / cos2Theta(w); }
float cosPhi(in const vec3 w) { return (sinTheta(w) == 0.0) ? 1.0 : clamp(w.x / sinTheta(w), -1.0, 1.0); }
float sinPhi(in const vec3 w) { return (sinTheta(w) == 0.0) ? 0.0 : clamp(w.y / sinTheta(w), -1.0, 1.0); }
float cos2Phi(in const vec3 w) { return cosPhi(w) * cosPhi(w); }
float sin2Phi(in const vec3 w) { return sinPhi(w) * sinPhi(w); }

vec3 reflect(in const vec3 w) { return vec3(-w.x, -w.y, w.z); }
vec3 refract(in const vec3 w, in float cosThetaT, in float eta)
{
    float scale = -(cosThetaT < 0.0 ? 1.0 / eta : eta);
    return vec3(scale * w.x, scale * w.y, cosThetaT);
}

const float GAMMA     = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

vec3 sRGBToLinear(in vec3 srgb)
{
    return pow(srgb.rgb, vec3(GAMMA));
}

vec3 linearTosRGB(in vec3 linear)
{
    return pow(linear.rgb, vec3(INV_GAMMA));
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
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}


#endif  // UTILS_GLSL