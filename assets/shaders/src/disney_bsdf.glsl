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

vec3 calculateColorTint(in vec3 baseColor)
{
    float lum = getLuminance(baseColor);
    return (lum > 0.0) ? baseColor / lum : vec3(1.0);
}

// Distribution Terms
// Custom terms created by Burley called Generalized Trowbridge-Reitz (GTR)

// clearcoat distribution term
float GTR1(in float hDotL, in float alpha)
{
    float aSqrd = alpha * alpha;
    float hDotLSqrd = hDotL * hDotL;

    float denom = PI * log(aSqrd) * (1.0 + (aSqrd - 1.0) * hDotLSqrd);

    return (aSqrd - 1.0) / denom;
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

// anisotropic specular G term -> Anisotropic Smith GGX Separable with fixed roughness of 0.25
// should fit GTR2
// https://jcgt.org/published/0003/02/03/paper.pdf
float GSmithSeparableAnisotropicGGX(in vec3 w, in float ax, in float ay)
{
    float aSqrd = cos2Phi(w) * ax * ax + sin2Phi(w) * ay * ay;
    float tanThetaSqrd = tan2Theta(w);
    return 2.0 / (1.0 + sqrt(1.0 + aSqrd * tanThetaSqrd));
}

// clearcoat specular G term -> Some seperable Smith term with fixed roughness of 0.25
// This does not fit the GTR1 term...
// https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
float GSmithSeparableBRDFViewer(in float nDotS, in float alpha)
{
    float aSqrd = alpha * alpha;
    float nDotSSqrd = nDotS * nDotS;
    return 1.0 / (nDotS + sqrt(aSqrd + nDotSSqrd - aSqrd * nDotSSqrd));
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


// sheen
vec3 evaluateDisneySheen()
{
    if(surfaceState.bsdf.sheen <= 0.0)
    {
        return vec3(0.0);
    }

    float hDotL = dot(surfaceState.wm, surfaceState.wi);

    vec3 colorTint = calculateColorTint(surfaceState.bsdf.baseColor);

    return surfaceState.bsdf.sheen * mix(vec3(1.0), colorTint, surfaceState.bsdf.sheenTint) * Schlick(hDotL);
}

// clearcoat
float evaluateDisneyClearcoat(out float pdf)
{
    if(surfaceState.bsdf.clearcoat <= 0.0)
    {
        return 0.0;
    }

    float nDotL = cosTheta(surfaceState.wi);
    float nDotV = cosTheta(surfaceState.wo);
    float nDotH = cosTheta(surfaceState.wm);
    float hDotL = dot(surfaceState.wm, surfaceState.wi);

    float D = GTR1(nDotH, mix(0.1, 0.001, surfaceState.bsdf.clearcoatGloss));
    float F = Fresnel(0.04, hDotL);
    float G = GSmithSeparableGGX(surfaceState.wi, 0.25) * GSmithSeparableGGX(surfaceState.wo, 0.25);

    pdf = 4.0 / (nDotL * nDotV);

    return surfaceState.bsdf.clearcoat * D * F * G;
}

// diffuse
float evaluateDisneyDiffuse(in bool thin)
{
    float nDotL = cosTheta(surfaceState.wi);
    float nDotV = cosTheta(surfaceState.wo);
    float hDotL = dot(surfaceState.wm, surfaceState.wi);

    float FL = Schlick(nDotL);
    float FV = Schlick(nDotV);

    // lambert
    float fLambert = 1.0;

    // thin
    // this is some ss approximation by Burley based on Hanrahan Krueger
    float fSSApprox = 0.0;
    if(thin && surfaceState.bsdf.flatness > 0.0)
    {
        float Fss90 = hDotL * hDotL * surfaceState.bsdf.roughness;
        float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
        float ss = 1.25 * (Fss * (1.0 / (nDotL + nDotL) - 0.5) + 0.5);
        fSSApprox = ss;
    }

    // float energyBias = mix(0.0, 0.5, surfaceState.bsdf.roughness) ;
    // float energyFactor = mix(1.0, 1.0 / 1.51, surfaceState.bsdf.roughness) ;
    float RR = /*energyBias + */2.0 * surfaceState.bsdf.roughness * hDotL * hDotL;

    // retro
    float fRetro = RR * (FL + FV + FL * FV * (RR - 1.0))/* * energyFactor*/;

    float diffuseF = mix(fLambert, fSSApprox, thin ? surfaceState.bsdf.flatness : 0.0);

    float fDisneyDiffuse = diffuseF * (1.0 - 0.5 * FL) * (1.0 - 0.5 * FV) + fRetro;

    return fDisneyDiffuse * INV_PI;
}

bool sampleDisneyDiffuse(out BSDFSample bsdfSample)
{
    bsdfSample.pdf = 1.0;
    bool thin = false; // FIXME
    float sgn = sign(cosTheta(surfaceState.wo));

    float u = rand();
    float v = rand();

    surfaceState.wi = sampleHemisphereCosine(bsdfSample.pdf);
    surfaceState.wm = normalize(surfaceState.wi + surfaceState.wo);

    float nDotL = cosTheta(surfaceState.wi);
    if(nDotL == 0.0)
    {
        bsdfSample.pdf = 0.0;
        bsdfSample.reflectance = vec3(0.0);
        return false;
    }

    vec3 color = surfaceState.bsdf.baseColor;

    float transPdf = 1.0;

    /*
    float p = rand();
    if(p <= surfaceState.bsdf.diffTrans)
    {
        surfaceState.wi = -surfaceState.wi;
        transPdf = surfaceState.bsdf.diffTrans;

        if(thin)
            color = sqrt(color);
        else {
            // FIXME
        }
    }
    else
    {
        transPdf = (1.0 - surfaceState.bsdf.diffTrans);
    }
    */

    vec3 sheen = evaluateDisneySheen();

    float diffuse = evaluateDisneyDiffuse(thin);

    bsdfSample.reflectance = sheen + color * diffuse / transPdf;
    bsdfSample.pdf *= transPdf;

    return true;
}

#endif // DISNEY_BSDF_GLSL