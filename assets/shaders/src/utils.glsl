#ifndef UTILS_GLSL
#define UTILS_GLSL

uint wangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint rngState;

uint xorshift32()
{
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);
    return rngState;
}

float rand()
{
    return uintBitsToFloat(xorshift32() >> 9 | 0x3f800000) - 1.0;
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

// beta = 2
float powerHeuristic(in int nf, in float fPdf, in int ng, in float gPdf) {
    float f = nf * fPdf, g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

vec3 tangentToWorld(in vec3 p, in vec3 tangent, in vec3 bitangent, in vec3 normal)
{
    return fma(vec3(p.x), tangent, fma(vec3(p.y), bitangent, p.z * normal));
}

vec3 worldToTangentFrame(in vec3 p, in vec3 tangent, in vec3 bitangent, in vec3 normal)
{
    return vec3(dot(p, tangent), dot(p, bitangent), dot(p, normal));
}

// tangent space trigonometry
float cosTheta(const vec3 w) {return w.z;}
float cos2Theta(const vec3 w) {return w.z * w.z;}
float sin2Theta(const vec3 w) {return max(0.0, 1.0 - cos2Theta(w));}
float sinTheta(const vec3 w) {return sqrt(sin2Theta(w));}
float tanTheta(const vec3 w) {return sinTheta(w) / cosTheta(w);}
float tan2Theta(const vec3 w) {return sin2Theta(w) / cos2Theta(w);}
float cosPhi(const vec3 w) {return (sinTheta(w) == 0.0) ? 1.0 : clamp(w.x / sinTheta(w), -1.0, 1.0);}
float sinPhi(const vec3 w) {return (sinTheta(w) == 0.0) ? 0.0 : clamp(w.y / sinTheta(w), -1.0, 1.0);}
float cos2Phi(const vec3 w) {return cosPhi(w) * cosPhi(w);}
float sin2Phi(const vec3 w) {return sinPhi(w) * sinPhi(w);}

vec3 sampleSphereUniform(in Sphere sphere)
{
    float z = 1.0 - 2.0 * rand();
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * rand();
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 sampleSphereSolidAngle(in vec3 point, in Sphere sphere, out float pdf)
{
        vec3 wDir = sphere.center - point;
        float dist = length(wDir);
        wDir = normalize(wDir);
        vec3 wDx, wDy;
        createCoordinateSystem(wDir, wDx, wDy);

        bool inside = (dist <= sphere.radius);
        if(inside)
        {
            pdf = 1.0 / (2.0 * TWO_PI * sphere.radius * sphere.radius);
            vec3 samplePoint = sampleSphereUniform(sphere);
            return sphere.center + samplePoint * sphere.radius;
        }

        // max cone angles
        float sinThetaMaxSqrd = sphere.radius * sphere.radius / (dist * dist);
        float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));
        pdf = 1.0 / (TWO_PI * (1.0 - cosThetaMax));
        // random in cone
        float rand0 = rand();
        float cosTheta = (1.0 - rand0) + rand0 * cosThetaMax;
        float sinThetaSqrd = max(0.0, 1.0 - cosTheta * cosTheta);
        float phi = TWO_PI * rand();
        // project to sphere surface
        float cosAlpha = dist / sphere.radius * sinThetaSqrd + cosTheta * sqrt(max(0.0, 1.0 - dist * dist / (sphere.radius * sphere.radius) * sinThetaSqrd));
        float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
        vec3 samplePoint = vec3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        samplePoint = tangentToWorld(samplePoint, -wDx, -wDy, -wDir);

        return sphere.center + samplePoint * sphere.radius;
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