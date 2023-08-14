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
    F0 + (1.0 - F0) * Schlick(x);
}

vec3 getColorTint(in vec3 baseColor)
{
    float luminance = getLuminance(baseColor);

    return luminance > 0.0 ? baseColor / luminance : vec3(1.0);
}

// sheen
vec3 evaluateSheen()
{
    if(surfaceState.bsdf.sheen <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 tint = getColorTint(surfaceState.bsdf.baseColor);
    return surfaceState.sheen * mix(vec3(1.0), tint, sheenTint) * Schlick(surfaceState.hDotL);
}


float GTR1(in float absNDotH, in float alpha)
{
    if(alpha >= 1.0)
    {
        return INV_PI;
    }

    float alphaSqrd = alpha * alpha;
    return (alphaSqrd - 1.0) * INV_PI / (log2(alphaSqrd) * (1.0 + (alphaSqrd - 1.0) * absNDotH * absNDotH));
}

float GSmithGGX(in float absNDotV, float alpha)
{
    float alphaSqrd = alpha * alpha;

    return 1.0 / (absNDotV + sqrt(alphaSqrd + (1.0 - alphaSqrd * absNDotV) * absNDotV));
}

// clearcoat
float evaluateClearcoat()
{
    if(surfaceState.clearcoat <= 0.0)
    {
        return 0.0;
    }

    float absNDotL = abs(surfaceState.nDotL);
    float absNDotV = abs(surfaceState.nDotV);
    float absNDotH = abs(surfaceState.nDotH);

    const float F0 = 0.04;

    float D = GTR1(absNDotH, surfaceState.clearcoatGloss);
    float F = Fresnel(F0, surfaceState.hDotL);
    float G = GSmithGGX(absNDotL, 0.25) * GSmithGGX(absNDotV, 0.25);

    float pdf = 0.25 / (absNDotL * absNDotV); // FIXME: Is this required?

    return surfaceState.clearcoat * D * F * G * pdf;
}



#endif // DISNEY_BSDF_GLSL