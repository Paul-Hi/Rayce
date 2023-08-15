#ifndef DISNEY_BSDF_GLSL
#define DISNEY_BSDF_GLSL

float getLuminance(in vec3 baseColor)
{
    // approximation
    return dot(vec3(0.3, 0.6, 1.0), baseColor);
}

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

vec3 evaluateDisneyDiffuse(out float pdf)
{
    pdf = 1.0;

    float FL = Schlick(surfaceState.nDotL);
    float FV = Schlick(surfaceState.nDotV);

    // lambert
    float fLambert = 1.0;

    float energyBias = mix(0.0, 0.5, surfaceState.bsdf.roughness) ;
    float energyFactor = mix(1.0, 1.0 / 1.51, surfaceState.bsdf.roughness) ;
    float RR = energyBias + 2.0 * surfaceState.bsdf.roughness * surfaceState.hDotL * surfaceState.hDotL;

    // retro
    float fRetro = RR * (FL + FV + FL * FV * (RR - 1.0)) * energyFactor;

    float fDisneyDiffuse = fLambert * (1.0 - 0.5 * FL) * (1.0 - 0.5 * FV) + fRetro;

    // 1 / pdf cancels out with  * INV_PI * surfaceState.nDotL
    // pdf = surfaceState.nDotL * INV_PI;
    return surfaceState.bsdf.baseColor * fDisneyDiffuse;
}


#endif // DISNEY_BSDF_GLSL