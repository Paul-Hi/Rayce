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

float FresnelDielectric(in float cosThetaI, in float eta, out float cosThetaT, out float etaIT, out float etaTI)
{
    const bool switchI = (cosThetaI < 0.0);
    etaIT = switchI ? 1.0 / eta : eta;
    etaTI = switchI ? eta : 1.0 / eta;

    if(eta == 1.0)
    {
        cosThetaT = -cosThetaI;
        return 0.0;
    }

    float cosThetaTSqrd = fnma(fnma(cosThetaI, cosThetaI, 1.0), etaTI * etaTI, 1.0);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        cosThetaT = 0.0;
        return 1.0;
    }

    float absCosThetaI = abs(cosThetaI);
    cosThetaT = sqrt(cosThetaTSqrd);

    float Rs = fnma(etaIT, cosThetaT, absCosThetaI) /  fma(etaIT, cosThetaT, absCosThetaI);
    float Rp =  fnma(etaIT, absCosThetaI, cosThetaT) /  fma(etaIT, absCosThetaI, cosThetaT);

    cosThetaT = !switchI ? -cosThetaT : cosThetaT;

    return 0.5 * (Rs * Rs + Rp * Rp);
}

float FresnelDielectric(in float cosThetaI, in float eta)
{
    float _ct, _eit, _eti;
    return FresnelDielectric(cosThetaI, eta, _ct, _eit, _eti);
}

// Geometry Terms

// clearcoat specular G term -> Basic Smith GGX Separable with fixed roughness of 0.25
// matching Heitz
// https://jcgt.org/published/0003/02/03/paper.pdf
float GSmithSeparableGGX(in vec3 w, in float alpha)
{
    float aSqrd = alpha * alpha;
    float tanThetaSqrd = tan2ThetaTS(w);
    return 2.0 / (1.0 + sqrt(1.0 + aSqrd * tanThetaSqrd));
}

// bsdfSample helper
vec3 sampleHemisphereCosine(inout float pdf)
{
    vec2 uv = rand2();
    float phi = TWO_PI * uv.x;
    float cosTheta = sqrt(1.0 - uv.y);
    float sinTheta = sqrt(uv.y);
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
        float nDotV = cosThetaTS(surfaceState.wo);
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
    else if(material.bsdfType == smoothDielectricThin)
    {
        return vec3(0.0, 0.0, 0.0); // FIXME: wrong, but should not happen
    }
}

float pdfBSDF(in Material material)
{
    if(material.bsdfType == diffuse)
    {
        float nDotV = cosThetaTS(surfaceState.wo);
        if(nDotV <= 0.0)
        {
            return 0.0;
        }
        return cosThetaTS(surfaceState.wi) / INV_PI;
    }
    else if(material.bsdfType == smoothDielectric)
    {
        return 0.0; // FIXME: wrong, but should not happen
    }
    else if(material.bsdfType == smoothDielectricThin)
    {
        return 0.0; // FIXME: wrong, but should not happen
    }
}

bool sampleBSDF(in Material material, out BSDFSample bsdfSample)
{
    bsdfSample.pdf = 1.0;

    if(material.bsdfType == diffuse)
    {
        bsdfSample.dirac = false;
        const float nDotV = cosThetaTS(surfaceState.wo);
        if(nDotV <= 0.0)
        {
            bsdfSample.pdf = 0.0;
            bsdfSample.reflectance = vec3(0.0);
            return false;
        }

        surfaceState.wi = sampleHemisphereCosine(bsdfSample.pdf);
        surfaceState.wm = normalize(surfaceState.wi + surfaceState.wo);

        const float nDotL = cosThetaTS(surfaceState.wi);
        if(nDotL <= 0.0)
        {
            bsdfSample.pdf = 0.0;
            bsdfSample.reflectance = vec3(0.0);
            return false;
        }

        bsdfSample.reflectance = evaluateLambert();

        return true;
    }
    else if(material.bsdfType == smoothDielectric)
    {
        bsdfSample.dirac = true;
        const float cosThetaI = cosThetaTS(surfaceState.wo);
        const float eta = surfaceState.bsdf.interiorIor / surfaceState.bsdf.exteriorIor;
        float cosThetaT, etaIT, etaTI;
        const float F = FresnelDielectric(cosThetaI, eta, cosThetaT, etaIT, etaTI);

        if (rand() <= F)
        {
            surfaceState.wi = reflectTS(surfaceState.wo);
            bsdfSample.pdf = F;

            bsdfSample.reflectance = surfaceState.bsdf.specularReflectance / abs(cosThetaTS(surfaceState.wi));

            return true;
        }
        else
        {
            surfaceState.wi = refractTS(surfaceState.wo, cosThetaT, etaTI);
            bsdfSample.pdf = 1.0 - F;

            bsdfSample.reflectance = etaTI * etaTI * surfaceState.bsdf.specularTransmittance / abs(cosThetaTS(surfaceState.wi));

            return true;
        }
    }
    else if(material.bsdfType == smoothDielectricThin)
    {
        bsdfSample.dirac = true;

        const float cosThetaI = cosThetaTS(surfaceState.wo);
        const float eta = surfaceState.bsdf.interiorIor / surfaceState.bsdf.exteriorIor;
        float F = FresnelDielectric(abs(cosThetaI), eta);
        // Account for internal reflections
        F *= 2.0 / (1.0 + F);

        if (rand() <= F)
        {
            surfaceState.wi = reflectTS(surfaceState.wo);
            bsdfSample.pdf = F;

            bsdfSample.reflectance = surfaceState.bsdf.specularReflectance / abs(cosThetaTS(surfaceState.wi));

            return true;
        }
        else
        {
            surfaceState.wi = -surfaceState.wo;
            bsdfSample.pdf = 1.0 - F;

            bsdfSample.reflectance = surfaceState.bsdf.specularTransmittance / abs(cosThetaTS(surfaceState.wi));

            return true;
        }
    }

    return false;
}

#endif // BSDF_GLSL
