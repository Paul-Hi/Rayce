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

vec3 evaluateDisneyDiffuse(in bool thin, out float pdf)
{
    pdf = 1.0;

    float FL = Schlick(surfaceState.nDotL);
    float FV = Schlick(surfaceState.nDotV);

    // lambert
    float fLambert = 1.0;

    // thin
    // this is some ss approximation by Burley based on Hanrahan Krueger
    float fSSApprox = 0.0;
    if(thin && surfaceState.bsdf.flatness > 0.0)
    {
        float Fss90 = surfaceState.hDotL * surfaceState.hDotL * surfaceState.bsdf.roughness;
        float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
        float ss = 1.25 * (Fss * (1.0 / (surfaceState.nDotL + surfaceState.nDotL) - 0.5) + 0.5);
        fSSApprox = ss;
    }

    float energyBias = mix(0.0, 0.5, surfaceState.bsdf.roughness) ;
    float energyFactor = mix(1.0, 1.0 / 1.51, surfaceState.bsdf.roughness) ;
    float RR = energyBias + 2.0 * surfaceState.bsdf.roughness * surfaceState.hDotL * surfaceState.hDotL;

    // retro
    float fRetro = RR * (FL + FV + FL * FV * (RR - 1.0)) * energyFactor;

    float diffuseF = mix(fLambert, fSSApprox, thin ? surfaceState.bsdf.flatness : 0.0);

    float fDisneyDiffuse = diffuseF * (1.0 - 0.5 * FL) * (1.0 - 0.5 * FV) + fRetro;

    // 1 / pdf cancels out with  * INV_PI * surfaceState.nDotL
    // pdf = surfaceState.nDotL * INV_PI;
    return surfaceState.bsdf.baseColor * fDisneyDiffuse;
}


#endif // DISNEY_BSDF_GLSL