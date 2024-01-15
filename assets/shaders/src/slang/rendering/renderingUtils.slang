import core.random;
__exported import core.utils;

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

// beta = 2 - number samples = 1
float powerHeuristic(const float fPdf, const float gPdf)
{
    const float fPdfSqrd = sqr(fPdf);
    return fPdfSqrd / (fPdfSqrd + sqr(gPdf));
}

float2 sampleUniformConcentricDisk(const float2 p)
{
    const float2 mapped = 2.0 * p - float2(1.0);
    if (mapped.x == 0.0 && mapped.y == 0.0)
    {
        return float2(0.0);
    }

    // concentric mapping
    float theta, r;
    if (abs(mapped.x) > abs(mapped.y))
    {
        r = mapped.x;
        theta = QUARTER_PI * (mapped.y / mapped.x);
    }
    else
    {
        r = mapped.y;
        theta = HALF_PI - QUARTER_PI * (mapped.x / mapped.y);
    }

    return r * float2(cos(theta), sin(theta));

}

float3 cosineHemisphereSample(const float2 uv = Random::rand2())
{
    const float2 d = sampleUniformConcentricDisk(uv);

    const float z = safe_sqrt(1.0 - sqr(d.x) - sqr(d.y));

    return float3(d.x, d.y, z);
}

float cosineHemispherePdf(const float cosTheta)
{
    return cosTheta * INV_PI;
}

float fresnelDielectric(const float cosThetaI, const float etati, const float etait)
{
    float eta = etati;
    float _ignore0 = 0.0;
    float _ignore1 = 0.0;
    return fresnelDielectric(cosThetaI, eta, _ignore0, _ignore1);
}

float fresnelDielectric(const float cosThetaI, inout float etati, out float etait, out float cosThetaT)
{
    if(etati == 1.0)
    {
        etait = etati;
        cosThetaT = -cosThetaI;
        return 0.0;
    }

    const bool switch = cosThetaI > 0.0;
    etait = switch ? etati : rcp(etati);
    etati = switch ? rcp(etati) : etati;

    const float cosThetaTSqrd = negativeMultiplyAndAdd(negativeMultiplyAndAdd(cosThetaI, cosThetaI, 1.0), sqr(etati), 1.0);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        cosThetaT = 0.0;
        return 1.0;
    }

    const float absCosThetaI = abs(cosThetaI);
    const float absCosThetaT = sqrt(cosThetaTSqrd);

    const float Rs = negativeMultiplyAndAdd(etait, absCosThetaT, absCosThetaI) / multiplyAndAdd(etait, absCosThetaT, absCosThetaI);
    const float Rp = negativeMultiplyAndAdd(etait, absCosThetaI, absCosThetaT) / multiplyAndAdd(etait, absCosThetaI, absCosThetaT);

    cosThetaT = switch ? -absCosThetaT : absCosThetaT;

    return 0.5 * (sqr(Rs) + sqr(Rp));
}

// Simliar to https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/#more-1921
float3 fresnelConductorExact(const float cosThetaI, const float3 eta, const float3 k)
{
    const float cos2ThetaI = sqr(cosThetaI);
    const float sin2ThetaI = 1.0 - cos2ThetaI;
    const float sin4ThetaI = sqr(sin2ThetaI);

    const float3 etaSqrd = sqr(eta);
    const float3 kSqrd = sqr(k);
    const float3 t0 = etaSqrd - kSqrd - sin2ThetaI;
    const float3 a2pb2 = safe_sqrt(t0 * t0 + 4.0 * kSqrd * etaSqrd);
    const float3 a = safe_sqrt(0.5 * (a2pb2 + t0));

    const float3 term1 = a2pb2 + cos2ThetaI;
    const float3 term2 = 2.0 * a * cosThetaI;

    const float3 Rs2 = (term1 - term2) / (term1 + term2);

    const float3 term3 = a2pb2 * cos2ThetaI + sin4ThetaI;
    const float3 term4 = term2 * sin2ThetaI;

    const float3 Rp2 = Rs2 * (term3 - term4) / (term3 + term4);

    return 0.5 * (Rp2 + Rs2);
}

float3 fresnelConductorApprox(const float cosThetaI, const float3 eta, const float3 k)
{
    const float cos2ThetaI = cosThetaI * cosThetaI;
    const float3 twoEtaCos2ThetaI = 2.0 * eta * cos2ThetaI;

    const float3 t0 = sqr(eta) + sqr(k);
    const float3 t1 = t0 * cos2ThetaI;
    const float3 Rs = (t0 - twoEtaCos2ThetaI + cos2ThetaI) / (t0 + twoEtaCos2ThetaI + cos2ThetaI);
    const float3 Rp = (t1 - twoEtaCos2ThetaI + 1.0) / (t1 + twoEtaCos2ThetaI + 1.0);

    return 0.5 * (Rp + Rs);
}

bool sameHemisphere(const float3 w, const float3 wp)
{
    const float cosTheta = cosThetaTS(w);
    const float cosThetaP = cosThetaTS(wp);

    return cosTheta * cosThetaP > 0.0;
}

bool sameHemisphere(const float cosTheta, const float cosThetaP)
{
    return cosTheta * cosThetaP > 0.0;
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
