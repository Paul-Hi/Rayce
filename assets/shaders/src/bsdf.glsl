#ifndef BSDF_GLSL
#define BSDF_GLSL

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


vec3 evaluateBSDF()
{
    // FIXME: Switch on type
    // diffuse -> lambert
    return surfaceState.bsdf.diffuseReflectance * INV_PI;
}

bool sampleBSDF(out BSDFSample bsdfSample)
{
    bsdfSample.pdf = 1.0;
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

    bsdfSample.reflectance = evaluateBSDF();

    return true;
}

#endif // BSDF_GLSL