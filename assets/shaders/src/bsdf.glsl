#ifndef BSDF_GLSL
#define BSDF_GLSL

#include "random.glsl"

float Schlick(in float x)
{
    float m = clamp(1.0 - x, 0.0, 1.0);
    float mSqrd = m * m;
    return mSqrd * mSqrd * m; // == m^5
}

float Fresnel(in float F0, in float x)
{
    return F0 + (1.0 - F0) * Schlick(x);
}

float FresnelDielectric(in float cosThetaI, in float eta, out float cosThetaT)
{
    if(eta == 1.0)
    {
        cosThetaT = -cosThetaI;
        return 0.0;
    }

    float scale = (cosThetaI < 0.0) ? 1.0 / eta : eta;
    float cosThetaTSqrd = 1.0 - (1.0 - cosThetaI * cosThetaI) * (scale * scale);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        cosThetaT = 0.0;
        return 1.0;
    }

    float absCosThetaI = abs(cosThetaI);
    cosThetaT = sqrt(cosThetaTSqrd);

    float Rs = (absCosThetaI - eta * cosThetaT) / (absCosThetaI + eta * cosThetaT);
    float Rp = (eta * absCosThetaI - cosThetaT) / (eta * absCosThetaI + cosThetaT);

    cosThetaT = (cosThetaI < 0.0) ? -cosThetaT : cosThetaT;

    return 0.5 * (Rs * Rs + Rp * Rp);
}

// Geometry Terms

// clearcoat specular G term -> Basic Smith GGX Separable with fixed roughness of 0.25
// matching Heitz
// https://jcgt.org/published/0003/02/03/paper.pdf
float GSmithSeparableGGX(in vec3 w, in float alpha)
{
    float aSqrd = alpha * alpha;
    float tanThetaSqrd = tan2Theta(w);
    return 2.0 / (1.0 + sqrt(1.0 + aSqrd * tanThetaSqrd));
}

// bsdfSample helper
vec3 sampleHemisphereCosine(inout float pdf)
{
    float u = rand();
    float v = rand();

    float phi = TWO_PI * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;

    pdf *= z * INV_PI;

    return vec3(x, y, z);
}

vec3 evaluateLambert()
{
    return surfaceState.bsdf.diffuseReflectance * INV_PI;
}

vec3 evaluateBSDF(in Material material)
{
    if(material.bsdfType == diffuse)
    {
        float nDotV = cosTheta(surfaceState.wo);
        if(nDotV <= 0.0)
        {
            return vec3(0.0, 0.0, 0.0);
        }
       return evaluateLambert();
    }
    else if(material.bsdfType == smoothDielectric)
    {
       return vec3(0.0, 0.0, 0.0); // FIXME: wrong, but should not happen
    }
}

float pdfBSDF(in Material material)
{
    if(material.bsdfType == diffuse)
    {
        float nDotV = cosTheta(surfaceState.wo);
        if(nDotV <= 0.0)
        {
            return 0.0;
        }
        return cosTheta(surfaceState.wi) / INV_PI;
    }
    else if(material.bsdfType == smoothDielectric)
    {
        return 0.0; // FIXME: wrong, but should not happen
    }
}

bool sampleBSDF(in Material material, out BSDFSample bsdfSample)
{
    bsdfSample.pdf = 1.0;

    if(material.bsdfType == diffuse)
    {
        float nDotV = cosTheta(surfaceState.wo);
        if(nDotV <= 0.0)
        {
            bsdfSample.pdf = 0.0;
            bsdfSample.reflectance = vec3(0.0);
            return false;
        }

        float u = rand();
        float v = rand();

        surfaceState.wi = sampleHemisphereCosine(bsdfSample.pdf);
        surfaceState.wm = normalize(surfaceState.wi + surfaceState.wo);

        float nDotL = cosTheta(surfaceState.wi);
        if(nDotL <= 0.0)
        {
            bsdfSample.pdf = 0.0;
            bsdfSample.reflectance = vec3(0.0);
            return false;
        }

        bsdfSample.reflectance = evaluateLambert();
    }
    else if(material.bsdfType == smoothDielectric)
    {
        // FIXME: Air - air interface darkens emitters?
        // FIXME: Strange 'bubble' for air - diamond or air water ...
        float s = rand();
        float cosThetaI = cosTheta(surfaceState.wo);
        float eta = surfaceState.bsdf.interiorIor / surfaceState.bsdf.exteriorIor;
        float cosThetaT;
        float F = FresnelDielectric(cosThetaI, eta, cosThetaT);

        if (s <= F)
        {
            surfaceState.wi = reflect(surfaceState.wo);
            bsdfSample.pdf = F;

            bsdfSample.reflectance = surfaceState.bsdf.specularReflectance / abs(cosTheta(surfaceState.wi));
        }
        else
        {
            surfaceState.wi = refract(surfaceState.wo, cosThetaT, eta);
            bsdfSample.pdf = 1.0 - F;

            bsdfSample.reflectance = surfaceState.bsdf.specularTransmittance / abs(cosTheta(surfaceState.wi));
        }
    }

    return true;
}

#endif // BSDF_GLSL