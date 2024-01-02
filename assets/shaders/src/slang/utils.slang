#pragma once

import globals;

// tangent space trigonometry
float cosThetaTS(const float3 w) { return w.z; }
float cos2ThetaTS(const float3 w) { return w.z * w.z; }
float sin2ThetaTS(const float3 w) { return max(0.0, 1.0 - cos2ThetaTS(w)); }
float sinThetaTS(const float3 w) { return sqrt(sin2ThetaTS(w)); }
float tanThetaTS(const float3 w) { return sinThetaTS(w) / cosThetaTS(w); }
float tan2ThetaTS(const float3 w) { return sin2ThetaTS(w) / cos2ThetaTS(w); }
float cosPhiTS(const float3 w) { return (sinThetaTS(w) == 0.0) ? 1.0 : clamp(w.x / sinThetaTS(w), -1.0, 1.0); }
float sinPhiTS(const float3 w) { return (sinThetaTS(w) == 0.0) ? 0.0 : clamp(w.y / sinThetaTS(w), -1.0, 1.0); }
float cos2PhiTS(const float3 w) { return cosPhiTS(w) * cosPhiTS(w); }
float sin2PhiTS(const float3 w) { return sinPhiTS(w) * sinPhiTS(w); }

float3 reflectTS(const float3 w) { return float3(-w.x, -w.y, w.z); }
float3 refractTS(const float3 w, float cosThetaT, float etati)
{
    return float3(-etati * w.x, -etati * w.y, cosThetaT);
}

float3 reflect(const float3 w, const float3 n)
{
    return 2.0 * dot(w, n) * n - w;
}

float3 refract(const float3 w, const float3 n, const float cosThetaT, const float etati)
{
    return (dot(w, n) * etati + cosThetaT) * n - w * etati;
}

func sqr<T : __BuiltinFloatingPointType>(T v) -> T
{
    return v * v;
}

func sqr<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> v) -> vector<T, N>
{
    return v * v;
}

func sqr<T : __BuiltinFloatingPointType, let N : int, let M : int>(matrix<T, N, M> v) -> matrix<T, N, M>
{
    return v * v;
}

func safe_sqrt<T : __BuiltinFloatingPointType>(T v) -> T
{
    return sqrt(max(T(1e-9), v)); // looks like slang sqrt(0) is nan?
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> v) -> vector<T, N>
{
    return sqrt(max(vector<T, N>(T(1e-9)), v)); // looks like slang sqrt(0) is nan?
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int, let M : int>(matrix<T, N, M> v) -> matrix<T, N, M>
{
    return sqrt(max(matrix<T, N, M>(T(1e-9)), v)); // looks like slang sqrt(0) is nan?
}

func multiplyAndAdd<T : __BuiltinFloatingPointType>(T a, T b, T c) -> T
{
    return a * b + c;
}

func multiplyAndAdd<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> a, vector<T, N> b, vector<T, N> c) -> vector<T, N>
{
    return a * b + c;
}

func negativeMultiplyAndAdd<T : __BuiltinFloatingPointType>(T a, T b, T c) -> T
{
    return -a * b + c;
}

func negativeMultiplyAndAdd<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> a, vector<T, N> b, vector<T, N> c) -> vector<T, N>
{
    return -a * b + c;
}

struct CoordinateSpace
{
    __init(const float3 normal, const bool sphere = false)
    {
        this.normal = normal;

        if(sphere)
        {
            // two ugly points but kind of consistent with uvs ...
            this.tangent = normalize(float3(-normal.z, 0.0, normal.x));
            this.bitangent = cross(normal, this.tangent);
        }
        else
        {
            // Not consistent for spheres
            // Building an Orthonormal Basis, Revisited, (JCGT), vol. 6, no. 1, 1-8, 2017
            // Available online: http://jcgt.org/published/0006/01/01/
            const float normalSign = normal.z < 0.0 ? -1.0 : 1.0;
            const float a = -rcp(normalSign + normal.z);
            const float b = normal.x * normal.y * a;
            this.tangent = float3(1.0 + normalSign * normal.x * normal.x * a, normalSign * b, -normalSign * normal.x);
            this.bitangent = float3(b, normalSign + normal.y * normal.y * a, -normal.y);}
    }

    __init(const float3 normal, bool canUsePartials, const float3 dfd1, const float3 dfd2, const float2 uvd1, const float2 uvd2)
    {
        this.normal = normal;
        if(canUsePartials && uvd1.x > 0.0)
        {
            const float3 t_    = (uvd2.y * dfd1 - uvd1.y * dfd2) / (uvd1.x * uvd2.y - uvd2.x * uvd1.y);
            this.tangent    = normalize(t_ - normal * dot(normal, t_));
            this.bitangent  = cross(normal, this.tangent);
            return this;
        }
        else
        {
            return CoordinateSpace(normal);
        }
    }

    CoordinateSpace flipped()
    {
        CoordinateSpace flippedSpace;
        flippedSpace.normal = -this.normal;
        flippedSpace.tangent = -this.tangent;
        flippedSpace.bitangent = -this.bitangent;
        return flippedSpace;
    }

    float3 tangentToWorld(const float3 p)
    {
        return multiplyAndAdd(float3(p.x), tangent, multiplyAndAdd(float3(p.y), bitangent, p.z * normal));
    }

    float3 worldToTangentFrame(const float3 p)
    {
        return float3(dot(p, tangent), dot(p, bitangent), dot(p, normal));
    }

    float3 normal;
    float3 tangent;
    float3 bitangent;
};

// beta = 2 - number samples = 1
float powerHeuristic(const float fPdf, const float gPdf)
{
    const float fPdfSqrd = sqr(fPdf);
    return fPdfSqrd / (fPdfSqrd + sqr(gPdf));
}

float2 mapToConcentricDisc(const float2 p)
{
    const float2 offset = p * 2.0 - 1.0;

    if(offset.x == 0 && offset.y == 0)
    {
        return float2(0.0);
    }

    float theta, r;
    if (abs(offset.x) > abs(offset.y))
    {
        r = offset.x;
        theta = QUARTER_PI * (offset.y / offset.x);
    }
    else
    {
        r = offset.y;
        theta = HALF_PI - QUARTER_PI * (offset.x / offset.y);
    }
    return r * float2(cos(theta), sin(theta));
}

static const float GAMMA     = 2.2;
static const float INV_GAMMA = 0.45454545454545454545454545454545;

float3 sRGBToLinear(const float3 srgb)
{
    return pow(srgb.rgb, float3(GAMMA));
}

float3 linearTosRGB(const float3 linear)
{
    return pow(linear.rgb, float3(INV_GAMMA));
}

#define RGB_TO_LUM float3(0.2125, 0.7154, 0.0721)

float averageLuminance(const float3 c)
{
    return dot(RGB_TO_LUM, c);
}

// ACES tone map
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 toneMapACES(const float3 color)
{
    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;
    return saturate((color * (A * color + B)) / (color * (C * color + D) + E));
}
